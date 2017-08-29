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

#include "mongoc-config.h"

#ifdef MONGOC_ENABLE_SSL

#include <errno.h>
#include <string.h>
#include <bson.h>

#include "mongoc-log.h"
#include "mongoc-trace-private.h"
#include "mongoc-error.h"

#include "mongoc-stream-tls-private.h"
#include "mongoc-stream-private.h"
#if defined(MONGOC_ENABLE_SSL_OPENSSL)
#include "mongoc-stream-tls-openssl.h"
#include "mongoc-openssl-private.h"
#elif defined(MONGOC_ENABLE_SSL_LIBRESSL)
#include "mongoc-libressl-private.h"
#include "mongoc-stream-tls-libressl.h"
#elif defined(MONGOC_ENABLE_SSL_SECURE_TRANSPORT)
#include "mongoc-secure-transport-private.h"
#include "mongoc-stream-tls-secure-transport.h"
#elif defined(MONGOC_ENABLE_SSL_SECURE_CHANNEL)
#include "mongoc-secure-channel-private.h"
#include "mongoc-stream-tls-secure-channel.h"
#endif
#include "mongoc-stream-tls.h"

#undef MONGOC_LOG_DOMAIN
#define MONGOC_LOG_DOMAIN "stream-tls"


/**
 * mongoc_stream_tls_handshake:
 *
 * Performs TLS handshake dance
 */
bool
mongoc_stream_tls_handshake (mongoc_stream_t *stream,
                             const char *host,
                             int32_t timeout_msec,
                             int *events,
                             bson_error_t *error)
{
   mongoc_stream_tls_t *stream_tls =
      (mongoc_stream_tls_t *) mongoc_stream_get_tls_stream (stream);

   BSON_ASSERT (stream_tls);
   BSON_ASSERT (stream_tls->handshake);

   stream_tls->timeout_msec = timeout_msec;

   return stream_tls->handshake (stream, host, events, error);
}

bool
mongoc_stream_tls_handshake_block (mongoc_stream_t *stream,
                                   const char *host,
                                   int32_t timeout_msec,
                                   bson_error_t *error)
{
   int events;
   ssize_t ret = 0;
   mongoc_stream_poll_t poller;
   int64_t now;
   int64_t expire = 0;

   if (timeout_msec >= 0) {
      expire = bson_get_monotonic_time () + (timeout_msec * 1000UL);
   }

   /*
    * error variables get re-used a lot. To prevent cross-contamination of error
    * messages, and still be able to provide a generic failure message when
    * mongoc_stream_tls_handshake fails without a specific reason, we need to
    * init
    * the error code to 0.
    */
   if (error) {
      error->code = 0;
   }
   do {
      events = 0;

      if (mongoc_stream_tls_handshake (
             stream, host, timeout_msec, &events, error)) {
         return true;
      }

      if (events) {
         poller.stream = stream;
         poller.events = events;
         poller.revents = 0;

         if (expire) {
            now = bson_get_monotonic_time ();
            if ((expire - now) < 0) {
               bson_set_error (error,
                               MONGOC_ERROR_STREAM,
                               MONGOC_ERROR_STREAM_SOCKET,
                               "TLS handshake timed out.");
               return false;
            } else {
               timeout_msec = (expire - now) / 1000L;
            }
         }
         ret = mongoc_stream_poll (&poller, 1, timeout_msec);
      }
   } while (events && ret > 0);

   if (error && !error->code) {
      bson_set_error (error,
                      MONGOC_ERROR_STREAM,
                      MONGOC_ERROR_STREAM_SOCKET,
                      "TLS handshake failed.");
   }
   return false;
}
/**
 * Deprecated. Was never supposed to be part of the public API.
 * See mongoc_stream_tls_handshake.
 */
bool
mongoc_stream_tls_do_handshake (mongoc_stream_t *stream, int32_t timeout_msec)
{
   mongoc_stream_tls_t *stream_tls =
      (mongoc_stream_tls_t *) mongoc_stream_get_tls_stream (stream);

   BSON_ASSERT (stream_tls);

   MONGOC_ERROR ("This function doesn't do anything. Please call "
                 "mongoc_stream_tls_handshake()");
   return false;
}


/**
 * Deprecated. Was never supposed to be part of the public API.
 * See mongoc_stream_tls_handshake.
 */
bool
mongoc_stream_tls_check_cert (mongoc_stream_t *stream, const char *host)
{
   mongoc_stream_tls_t *stream_tls =
      (mongoc_stream_tls_t *) mongoc_stream_get_tls_stream (stream);

   BSON_ASSERT (stream_tls);

   MONGOC_ERROR ("This function doesn't do anything. Please call "
                 "mongoc_stream_tls_handshake()");
   return false;
}


/*
 *--------------------------------------------------------------------------
 *
 * mongoc_stream_tls_new_with_hostname --
 *
 *       Creates a new mongoc_stream_tls_t to communicate with a remote
 *       server using a TLS stream.
 *
 *       @host the hostname we are connected to and to verify the
 *       server certificate against
 *
 *       @base_stream should be a stream that will become owned by the
 *       resulting tls stream. It will be used for raw I/O.
 *
 *       @trust_store_dir should be a path to the SSL cert db to use for
 *       verifying trust of the remote server.
 *
 * Returns:
 *       NULL on failure, otherwise a mongoc_stream_t.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

mongoc_stream_t *
mongoc_stream_tls_new_with_hostname (mongoc_stream_t *base_stream,
                                     const char *host,
                                     mongoc_ssl_opt_t *opt,
                                     int client)
{
   BSON_ASSERT (base_stream);

   /* !client is only used for testing,
    * when the streams are pretending to be the server */
   if (!client || opt->weak_cert_validation) {
      opt->allow_invalid_hostname = true;
   }

#ifndef _WIN32
   /* Silly check for Unix Domain Sockets */
   if (!host || (host[0] == '/' && !access (host, F_OK))) {
      opt->allow_invalid_hostname = true;
   }
#endif

#if defined(MONGOC_ENABLE_SSL_OPENSSL)
   return mongoc_stream_tls_openssl_new (base_stream, host, opt, client);
#elif defined(MONGOC_ENABLE_SSL_LIBRESSL)
   return mongoc_stream_tls_libressl_new (base_stream, host, opt, client);
#elif defined(MONGOC_ENABLE_SSL_SECURE_TRANSPORT)
   return mongoc_stream_tls_secure_transport_new (
      base_stream, host, opt, client);
#elif defined(MONGOC_ENABLE_SSL_SECURE_CHANNEL)
   return mongoc_stream_tls_secure_channel_new (base_stream, host, opt, client);
#else
#error "Don't know how to create TLS stream"
#endif
}

mongoc_stream_t *
mongoc_stream_tls_new (mongoc_stream_t *base_stream,
                       mongoc_ssl_opt_t *opt,
                       int client)
{
   return mongoc_stream_tls_new_with_hostname (base_stream, NULL, opt, client);
}

#endif
