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


#include <bson.h>

#include "mongoc-config.h"
#include "mongoc-counters-private.h"
#include "mongoc-init.h"

#include "mongoc-handshake-private.h"

#ifdef MONGOC_ENABLE_SSL
#include "mongoc-scram-private.h"
#include "mongoc-ssl.h"
#ifdef MONGOC_ENABLE_SSL_OPENSSL
#include "mongoc-openssl-private.h"
#elif defined(MONGOC_ENABLE_SSL_LIBRESSL)
#include "tls.h"
#endif
#endif
#include "mongoc-thread-private.h"
#include "mongoc-trace-private.h"


#ifndef MONGOC_NO_AUTOMATIC_GLOBALS
#pragma message( \
   "Configure the driver with --disable-automatic-init-and-cleanup\
 (if using ./configure) or ENABLE_AUTOMATIC_INIT_AND_CLEANUP=OFF (with cmake).\
 Automatic cleanup is deprecated and will be removed in version 2.0.")
#endif

#ifdef MONGOC_ENABLE_SASL_CYRUS
#include <sasl/sasl.h>
#include "mongoc-cyrus-private.h"

static void *
mongoc_cyrus_mutex_alloc (void)
{
   mongoc_mutex_t *mutex;

   mutex = (mongoc_mutex_t *) bson_malloc0 (sizeof (mongoc_mutex_t));
   mongoc_mutex_init (mutex);

   return (void *) mutex;
}


static int
mongoc_cyrus_mutex_lock (void *mutex)
{
   mongoc_mutex_lock ((mongoc_mutex_t *) mutex);

   return SASL_OK;
}


static int
mongoc_cyrus_mutex_unlock (void *mutex)
{
   mongoc_mutex_unlock ((mongoc_mutex_t *) mutex);

   return SASL_OK;
}


static void
mongoc_cyrus_mutex_free (void *mutex)
{
   mongoc_mutex_destroy ((mongoc_mutex_t *) mutex);
   bson_free (mutex);
}

#endif /* MONGOC_ENABLE_SASL_CYRUS */


static MONGOC_ONCE_FUN (_mongoc_do_init)
{
#ifdef MONGOC_ENABLE_SASL_CYRUS
   int status;
   sasl_callback_t callbacks[] = {
      {SASL_CB_LOG, SASL_CALLBACK_FN (_mongoc_cyrus_log), NULL},
      {SASL_CB_LIST_END}};
#endif
#ifdef MONGOC_ENABLE_SSL_OPENSSL
   _mongoc_openssl_init ();
#elif defined(MONGOC_ENABLE_SSL_LIBRESSL)
   tls_init ();
#endif

#ifdef MONGOC_ENABLE_SSL
   _mongoc_scram_startup ();
#endif

#ifdef MONGOC_ENABLE_SASL_CYRUS
   /* The following functions should not use tracing, as they may be invoked
    * before mongoc_log_set_handler() can complete. */
   sasl_set_mutex (mongoc_cyrus_mutex_alloc,
                   mongoc_cyrus_mutex_lock,
                   mongoc_cyrus_mutex_unlock,
                   mongoc_cyrus_mutex_free);

   status = sasl_client_init (callbacks);
   BSON_ASSERT (status == SASL_OK);
#endif

   _mongoc_counters_init ();

#ifdef _WIN32
   {
      WORD wVersionRequested;
      WSADATA wsaData;
      int err;

      wVersionRequested = MAKEWORD (2, 2);

      err = WSAStartup (wVersionRequested, &wsaData);

      /* check the version perhaps? */

      BSON_ASSERT (err == 0);
   }
#endif

   _mongoc_handshake_init ();

   MONGOC_ONCE_RETURN;
}

void
mongoc_init (void)
{
   static mongoc_once_t once = MONGOC_ONCE_INIT;
   mongoc_once (&once, _mongoc_do_init);
}

static MONGOC_ONCE_FUN (_mongoc_do_cleanup)
{
#ifdef MONGOC_ENABLE_SSL_OPENSSL
   _mongoc_openssl_cleanup ();
#endif

#ifdef MONGOC_ENABLE_SASL_CYRUS
#ifdef MONGOC_HAVE_SASL_CLIENT_DONE
   sasl_client_done ();
#else
   /* fall back to deprecated function */
   sasl_done ();
#endif
#endif

#ifdef _WIN32
   WSACleanup ();
#endif

   _mongoc_counters_cleanup ();

   _mongoc_handshake_cleanup ();

   MONGOC_ONCE_RETURN;
}

void
mongoc_cleanup (void)
{
   static mongoc_once_t once = MONGOC_ONCE_INIT;
   mongoc_once (&once, _mongoc_do_cleanup);
}

/*
 * On GCC, just use __attribute__((constructor)) to perform initialization
 * automatically for the application.
 */
#if defined(__GNUC__) && !defined(MONGOC_NO_AUTOMATIC_GLOBALS)
static void
_mongoc_init_ctor (void) __attribute__ ((constructor));
static void
_mongoc_init_ctor (void)
{
   mongoc_init ();
}

static void
_mongoc_init_dtor (void) __attribute__ ((destructor));
static void
_mongoc_init_dtor (void)
{
   bson_mem_restore_vtable ();
   mongoc_cleanup ();
}
#endif
