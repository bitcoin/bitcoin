/*
 * Copyright 2015 MongoDB, Inc.
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

#include <mongoc.h>
#include <mongoc-thread-private.h>
#ifdef MONGOC_ENABLE_SASL_CYRUS
#include <mongoc-cyrus-private.h>
#endif

#include "TestSuite.h"
#include "test-libmongoc.h"


static const char *GSSAPI_HOST = "MONGOC_TEST_GSSAPI_HOST";
static const char *GSSAPI_USER = "MONGOC_TEST_GSSAPI_USER";

#define NTHREADS 10
#define NLOOPS 10


int
should_run_gssapi_kerberos (void)
{
   char *host = test_framework_getenv (GSSAPI_HOST);
   char *user = test_framework_getenv (GSSAPI_USER);
   int ret = (host && user);

   bson_free (host);
   bson_free (user);

   return ret;
}


struct closure_t {
   mongoc_client_pool_t *pool;
   int finished;
   mongoc_mutex_t mutex;
};


static void *
gssapi_kerberos_worker (void *data)
{
   struct closure_t *closure = (struct closure_t *) data;
   mongoc_client_pool_t *pool = closure->pool;
   mongoc_client_t *client;
   mongoc_collection_t *collection;
   mongoc_cursor_t *cursor;
   bson_error_t error;
   const bson_t *doc;
   bson_t query = BSON_INITIALIZER;
   int i;

   for (i = 0; i < NLOOPS; i++) {
      client = mongoc_client_pool_pop (pool);
      collection = mongoc_client_get_collection (client, "kerberos", "test");
      cursor = mongoc_collection_find (
         collection, MONGOC_QUERY_NONE, 0, 0, 0, &query, NULL, NULL);

      if (!mongoc_cursor_next (cursor, &doc) &&
          mongoc_cursor_error (cursor, &error)) {
         fprintf (stderr, "Cursor Failure: %s\n", error.message);
         abort ();
      }

      mongoc_cursor_destroy (cursor);
      mongoc_collection_destroy (collection);
      mongoc_client_pool_push (pool, client);
   }

   bson_destroy (&query);

   mongoc_mutex_lock (&closure->mutex);
   closure->finished++;
   mongoc_mutex_unlock (&closure->mutex);

   return NULL;
}


static void
test_gssapi_kerberos (void *context)
{
   char *host = test_framework_getenv (GSSAPI_HOST);
   char *user = test_framework_getenv (GSSAPI_USER);
   char *uri_str;
   mongoc_uri_t *uri;
   struct closure_t closure = {0};
   int i;
   mongoc_thread_t threads[NTHREADS];

   BSON_ASSERT (host && user);

   mongoc_mutex_init (&closure.mutex);

   uri_str = bson_strdup_printf (
      "mongodb://%s@%s/?authMechanism=GSSAPI&serverselectiontimeoutms=1000",
      user,
      host);

   uri = mongoc_uri_new (uri_str);
   closure.pool = mongoc_client_pool_new (uri);

   for (i = 0; i < NTHREADS; i++) {
      mongoc_thread_create (
         &threads[i], gssapi_kerberos_worker, (void *) &closure);
   }

   for (i = 0; i < NTHREADS; i++) {
      mongoc_thread_join (threads[i]);
   }

   mongoc_mutex_lock (&closure.mutex);
   ASSERT_CMPINT (NTHREADS, ==, closure.finished);
   mongoc_mutex_unlock (&closure.mutex);

   mongoc_client_pool_destroy (closure.pool);
   mongoc_mutex_destroy (&closure.mutex);
   mongoc_uri_destroy (uri);
   bson_free (uri_str);
   bson_free (host);
   bson_free (user);
}


#ifdef MONGOC_ENABLE_SASL_CYRUS
static void
test_sasl_properties (void)
{
   mongoc_uri_t *uri;
   mongoc_cyrus_t sasl;

   uri = mongoc_uri_new (
      "mongodb://host/?authMechanism=GSSAPI&"
      "authMechanismProperties=SERVICE_NAME:sn,CANONICALIZE_HOST_NAME:TrUe");

   memset (&sasl, 0, sizeof sasl);
   _mongoc_sasl_set_properties ((mongoc_sasl_t *)&sasl, uri);

   ASSERT (sasl.credentials.canonicalize_host_name);
   ASSERT_CMPSTR (sasl.credentials.service_name, "sn");

   mongoc_uri_destroy (uri);

   /* authMechanismProperties take precedence */
   uri = mongoc_uri_new (
      "mongodb://host/?authMechanism=GSSAPI&"
      "canonicalizeHostname=true&gssapiServiceName=blah&"
      "authMechanismProperties=SERVICE_NAME:sn,CANONICALIZE_HOST_NAME:False");

   _mongoc_cyrus_destroy (&sasl);
   memset (&sasl, 0, sizeof sasl);
   _mongoc_sasl_set_properties ((mongoc_sasl_t *)&sasl, uri);

   ASSERT (!sasl.credentials.canonicalize_host_name);
   ASSERT_CMPSTR (sasl.credentials.service_name, "sn");

   _mongoc_cyrus_destroy (&sasl);
   mongoc_uri_destroy (uri);
}
#endif


void
test_sasl_install (TestSuite *suite)
{
   TestSuite_AddFull (suite,
                      "/SASL/gssapi_kerberos",
                      test_gssapi_kerberos,
                      NULL,
                      NULL,
                      should_run_gssapi_kerberos);
#ifdef MONGOC_ENABLE_SASL_CYRUS
   TestSuite_Add (suite, "/SASL/properties", test_sasl_properties);
#endif
}
