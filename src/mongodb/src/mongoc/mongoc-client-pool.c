/*
 * Copyright 2013 MongoDB, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#include "mongoc.h"
#include "mongoc-apm-private.h"
#include "mongoc-counters-private.h"
#include "mongoc-client-pool-private.h"
#include "mongoc-client-pool.h"
#include "mongoc-client-private.h"
#include "mongoc-queue-private.h"
#include "mongoc-thread-private.h"
#include "mongoc-topology-private.h"
#include "mongoc-trace-private.h"

#ifdef MONGOC_ENABLE_SSL
#include "mongoc-ssl-private.h"
#endif

struct _mongoc_client_pool_t {
   mongoc_mutex_t mutex;
   mongoc_cond_t cond;
   mongoc_queue_t queue;
   mongoc_topology_t *topology;
   mongoc_uri_t *uri;
   uint32_t min_pool_size;
   uint32_t max_pool_size;
   uint32_t size;
#ifdef MONGOC_ENABLE_SSL
   bool ssl_opts_set;
   mongoc_ssl_opt_t ssl_opts;
#endif
   bool apm_callbacks_set;
   mongoc_apm_callbacks_t apm_callbacks;
   void *apm_context;
   int32_t error_api_version;
   bool error_api_set;
};


#ifdef MONGOC_ENABLE_SSL
void
mongoc_client_pool_set_ssl_opts (mongoc_client_pool_t *pool,
                                 const mongoc_ssl_opt_t *opts)
{
   BSON_ASSERT (pool);

   mongoc_mutex_lock (&pool->mutex);

   _mongoc_ssl_opts_cleanup (&pool->ssl_opts);

   memset (&pool->ssl_opts, 0, sizeof pool->ssl_opts);
   pool->ssl_opts_set = false;

   if (opts) {
      _mongoc_ssl_opts_copy_to (opts, &pool->ssl_opts);
      pool->ssl_opts_set = true;
   }

   mongoc_topology_scanner_set_ssl_opts (pool->topology->scanner,
                                         &pool->ssl_opts);

   mongoc_mutex_unlock (&pool->mutex);
}
#endif


mongoc_client_pool_t *
mongoc_client_pool_new (const mongoc_uri_t *uri)
{
   mongoc_topology_t *topology;
   mongoc_client_pool_t *pool;
   const bson_t *b;
   bson_iter_t iter;
   const char *appname;


   ENTRY;

   BSON_ASSERT (uri);

#ifndef MONGOC_ENABLE_SSL
   if (mongoc_uri_get_ssl (uri)) {
      MONGOC_ERROR ("Can't create SSL client pool,"
                    " SSL not enabled in this build.");
      return NULL;
   }
#endif

   pool = (mongoc_client_pool_t *) bson_malloc0 (sizeof *pool);
   mongoc_mutex_init (&pool->mutex);
   _mongoc_queue_init (&pool->queue);
   pool->uri = mongoc_uri_copy (uri);
   pool->min_pool_size = 0;
   pool->max_pool_size = 100;
   pool->size = 0;

   topology = mongoc_topology_new (uri, false);
   pool->topology = topology;
   pool->error_api_version = MONGOC_ERROR_API_VERSION_LEGACY;

   b = mongoc_uri_get_options (pool->uri);

   if (bson_iter_init_find_case (&iter, b, MONGOC_URI_MINPOOLSIZE)) {
      if (BSON_ITER_HOLDS_INT32 (&iter)) {
         pool->min_pool_size = BSON_MAX (0, bson_iter_int32 (&iter));
      }
   }

   if (bson_iter_init_find_case (&iter, b, MONGOC_URI_MAXPOOLSIZE)) {
      if (BSON_ITER_HOLDS_INT32 (&iter)) {
         pool->max_pool_size = BSON_MAX (1, bson_iter_int32 (&iter));
      }
   }

   appname =
      mongoc_uri_get_option_as_utf8 (pool->uri, MONGOC_URI_APPNAME, NULL);
   if (appname) {
      /* the appname should have already been validated */
      BSON_ASSERT (mongoc_client_pool_set_appname (pool, appname));
   }

#ifdef MONGOC_ENABLE_SSL
   if (mongoc_uri_get_ssl (pool->uri)) {
      mongoc_ssl_opt_t ssl_opt = {0};

      _mongoc_ssl_opts_from_uri (&ssl_opt, pool->uri);
      /* sets use_ssl = true */
      mongoc_client_pool_set_ssl_opts (pool, &ssl_opt);
   }
#endif
   mongoc_counter_client_pools_active_inc ();

   RETURN (pool);
}


void
mongoc_client_pool_destroy (mongoc_client_pool_t *pool)
{
   mongoc_client_t *client;

   ENTRY;

   BSON_ASSERT (pool);

   while (
      (client = (mongoc_client_t *) _mongoc_queue_pop_head (&pool->queue))) {
      mongoc_client_destroy (client);
   }

   mongoc_topology_destroy (pool->topology);

   mongoc_uri_destroy (pool->uri);
   mongoc_mutex_destroy (&pool->mutex);
   mongoc_cond_destroy (&pool->cond);

#ifdef MONGOC_ENABLE_SSL
   _mongoc_ssl_opts_cleanup (&pool->ssl_opts);
#endif

   bson_free (pool);

   mongoc_counter_client_pools_active_dec ();
   mongoc_counter_client_pools_disposed_inc ();

   EXIT;
}


/*
 * Start the background topology scanner.
 *
 * This function assumes the pool's mutex is locked
 */
static void
_start_scanner_if_needed (mongoc_client_pool_t *pool)
{
   if (!_mongoc_topology_start_background_scanner (pool->topology)) {
      MONGOC_ERROR ("Background scanner did not start!");
      abort ();
   }
}

mongoc_client_t *
mongoc_client_pool_pop (mongoc_client_pool_t *pool)
{
   mongoc_client_t *client;

   ENTRY;

   BSON_ASSERT (pool);

   mongoc_mutex_lock (&pool->mutex);

again:
   if (!(client = (mongoc_client_t *) _mongoc_queue_pop_head (&pool->queue))) {
      if (pool->size < pool->max_pool_size) {
         client = _mongoc_client_new_from_uri (pool->uri, pool->topology);

         /* for tests */
         mongoc_client_set_stream_initiator (
            client,
            pool->topology->scanner->initiator,
            pool->topology->scanner->initiator_context);

         client->error_api_version = pool->error_api_version;
         _mongoc_client_set_apm_callbacks_private (
            client, &pool->apm_callbacks, pool->apm_context);
#ifdef MONGOC_ENABLE_SSL
         if (pool->ssl_opts_set) {
            mongoc_client_set_ssl_opts (client, &pool->ssl_opts);
         }
#endif
         pool->size++;
      } else {
         mongoc_cond_wait (&pool->cond, &pool->mutex);
         GOTO (again);
      }
   }

   _start_scanner_if_needed (pool);
   mongoc_mutex_unlock (&pool->mutex);

   RETURN (client);
}


mongoc_client_t *
mongoc_client_pool_try_pop (mongoc_client_pool_t *pool)
{
   mongoc_client_t *client;

   ENTRY;

   BSON_ASSERT (pool);

   mongoc_mutex_lock (&pool->mutex);

   if (!(client = (mongoc_client_t *) _mongoc_queue_pop_head (&pool->queue))) {
      if (pool->size < pool->max_pool_size) {
         client = _mongoc_client_new_from_uri (pool->uri, pool->topology);
#ifdef MONGOC_ENABLE_SSL
         if (pool->ssl_opts_set) {
            mongoc_client_set_ssl_opts (client, &pool->ssl_opts);
         }
#endif
         pool->size++;
      }
   }

   if (client) {
      _start_scanner_if_needed (pool);
   }
   mongoc_mutex_unlock (&pool->mutex);

   RETURN (client);
}


void
mongoc_client_pool_push (mongoc_client_pool_t *pool, mongoc_client_t *client)
{
   ENTRY;

   BSON_ASSERT (pool);
   BSON_ASSERT (client);

   mongoc_mutex_lock (&pool->mutex);
   _mongoc_queue_push_head (&pool->queue, client);

   if (pool->min_pool_size &&
       _mongoc_queue_get_length (&pool->queue) > pool->min_pool_size) {
      mongoc_client_t *old_client;
      old_client = (mongoc_client_t *) _mongoc_queue_pop_tail (&pool->queue);
      if (old_client) {
         mongoc_client_destroy (old_client);
         pool->size--;
      }
   }

   mongoc_cond_signal (&pool->cond);
   mongoc_mutex_unlock (&pool->mutex);

   EXIT;
}

/* for tests */
void
_mongoc_client_pool_set_stream_initiator (mongoc_client_pool_t *pool,
                                          mongoc_stream_initiator_t si,
                                          void *context)
{
   mongoc_topology_scanner_set_stream_initiator (
      pool->topology->scanner, si, context);
}

/* for tests */
size_t
mongoc_client_pool_get_size (mongoc_client_pool_t *pool)
{
   size_t size = 0;

   ENTRY;

   mongoc_mutex_lock (&pool->mutex);
   size = pool->size;
   mongoc_mutex_unlock (&pool->mutex);

   RETURN (size);
}


size_t
mongoc_client_pool_num_pushed (mongoc_client_pool_t *pool)
{
   size_t num_pushed = 0;

   ENTRY;

   mongoc_mutex_lock (&pool->mutex);
   num_pushed = pool->queue.length;
   mongoc_mutex_unlock (&pool->mutex);

   RETURN (num_pushed);
}


mongoc_topology_t *
_mongoc_client_pool_get_topology (mongoc_client_pool_t *pool)
{
   return pool->topology;
}


void
mongoc_client_pool_max_size (mongoc_client_pool_t *pool, uint32_t max_pool_size)
{
   ENTRY;

   mongoc_mutex_lock (&pool->mutex);
   pool->max_pool_size = max_pool_size;
   mongoc_mutex_unlock (&pool->mutex);

   EXIT;
}

void
mongoc_client_pool_min_size (mongoc_client_pool_t *pool, uint32_t min_pool_size)
{
   ENTRY;

   mongoc_mutex_lock (&pool->mutex);
   pool->min_pool_size = min_pool_size;
   mongoc_mutex_unlock (&pool->mutex);

   EXIT;
}

bool
mongoc_client_pool_set_apm_callbacks (mongoc_client_pool_t *pool,
                                      mongoc_apm_callbacks_t *callbacks,
                                      void *context)
{
   mongoc_topology_t *topology;

   topology = pool->topology;

   if (pool->apm_callbacks_set) {
      MONGOC_ERROR ("Can only set callbacks once");
      return false;
   }

   mongoc_mutex_lock (&topology->mutex);

   if (callbacks) {
      memcpy (&topology->description.apm_callbacks,
              callbacks,
              sizeof (mongoc_apm_callbacks_t));
      memcpy (&pool->apm_callbacks, callbacks, sizeof (mongoc_apm_callbacks_t));
   }

   mongoc_topology_set_apm_callbacks (topology, callbacks, context);
   topology->description.apm_context = context;
   pool->apm_context = context;
   pool->apm_callbacks_set = true;

   mongoc_mutex_unlock (&topology->mutex);

   return true;
}

bool
mongoc_client_pool_set_error_api (mongoc_client_pool_t *pool, int32_t version)
{
   if (version != MONGOC_ERROR_API_VERSION_LEGACY &&
       version != MONGOC_ERROR_API_VERSION_2) {
      MONGOC_ERROR ("Unsupported Error API Version: %" PRId32, version);
      return false;
   }

   if (pool->error_api_set) {
      MONGOC_ERROR ("Can only set Error API Version once");
      return false;
   }

   pool->error_api_version = version;
   pool->error_api_set = true;

   return true;
}

bool
mongoc_client_pool_set_appname (mongoc_client_pool_t *pool, const char *appname)
{
   bool ret;

   mongoc_mutex_lock (&pool->mutex);
   ret = _mongoc_topology_set_appname (pool->topology, appname);
   mongoc_mutex_unlock (&pool->mutex);

   return ret;
}
