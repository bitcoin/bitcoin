/*
 * Copyright 2016 MongoDB, Inc.
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

#ifdef MONGOC_ENABLE_SSL_SECURE_TRANSPORT

#include <Security/Security.h>
#include <Security/SecureTransport.h>
#include <CoreFoundation/CoreFoundation.h>


#include <bson.h>

#include "mongoc-trace-private.h"
#include "mongoc-log.h"
#include "mongoc-secure-transport-private.h"
#include "mongoc-ssl.h"
#include "mongoc-error.h"
#include "mongoc-counters-private.h"
#include "mongoc-stream-tls.h"
#include "mongoc-stream-tls-private.h"
#include "mongoc-stream-private.h"
#include "mongoc-stream-tls-secure-transport-private.h"

#undef MONGOC_LOG_DOMAIN
#define MONGOC_LOG_DOMAIN "stream-tls-secure_transport"

static void
_mongoc_stream_tls_secure_transport_destroy (mongoc_stream_t *stream)
{
   mongoc_stream_tls_t *tls = (mongoc_stream_tls_t *) stream;
   mongoc_stream_tls_secure_transport_t *secure_transport =
      (mongoc_stream_tls_secure_transport_t *) tls->ctx;

   ENTRY;
   BSON_ASSERT (secure_transport);

   SSLClose (secure_transport->ssl_ctx_ref);
   CFRelease (secure_transport->ssl_ctx_ref);
   secure_transport->ssl_ctx_ref = NULL;

   /* SSLClose will do IO so destroy must come after */
   mongoc_stream_destroy (tls->base_stream);

   if (secure_transport->anchors) {
      CFRelease (secure_transport->anchors);
   }
   if (secure_transport->my_cert) {
      CFRelease (secure_transport->my_cert);
   }
   bson_free (secure_transport);
   bson_free (stream);

   mongoc_counter_streams_active_dec ();
   mongoc_counter_streams_disposed_inc ();
   EXIT;
}

static void
_mongoc_stream_tls_secure_transport_failed (mongoc_stream_t *stream)
{
   ENTRY;
   _mongoc_stream_tls_secure_transport_destroy (stream);
   EXIT;
}

static int
_mongoc_stream_tls_secure_transport_close (mongoc_stream_t *stream)
{
   int ret = 0;
   mongoc_stream_tls_t *tls = (mongoc_stream_tls_t *) stream;
   mongoc_stream_tls_secure_transport_t *secure_transport =
      (mongoc_stream_tls_secure_transport_t *) tls->ctx;

   ENTRY;
   BSON_ASSERT (secure_transport);

   ret = mongoc_stream_close (tls->base_stream);
   RETURN (ret);
}

static int
_mongoc_stream_tls_secure_transport_flush (mongoc_stream_t *stream)
{
   mongoc_stream_tls_t *tls = (mongoc_stream_tls_t *) stream;
   mongoc_stream_tls_secure_transport_t *secure_transport =
      (mongoc_stream_tls_secure_transport_t *) tls->ctx;

   ENTRY;
   BSON_ASSERT (secure_transport);
   RETURN (0);
}

static ssize_t
_mongoc_stream_tls_secure_transport_write (mongoc_stream_t *stream,
                                           char *buf,
                                           size_t buf_len)
{
   OSStatus status;
   mongoc_stream_tls_t *tls = (mongoc_stream_tls_t *) stream;
   mongoc_stream_tls_secure_transport_t *secure_transport =
      (mongoc_stream_tls_secure_transport_t *) tls->ctx;
   ssize_t write_ret;
   int64_t now;
   int64_t expire = 0;

   ENTRY;
   BSON_ASSERT (secure_transport);

   if (tls->timeout_msec >= 0) {
      expire = bson_get_monotonic_time () + (tls->timeout_msec * 1000UL);
   }

   status = SSLWrite (
      secure_transport->ssl_ctx_ref, buf, buf_len, (size_t *) &write_ret);

   switch (status) {
   case errSSLWouldBlock:
   case noErr:
      break;

   case errSSLClosedAbort:
      errno = ECONNRESET;

   default:
      RETURN (-1);
   }

   if (expire) {
      now = bson_get_monotonic_time ();

      if ((expire - now) < 0) {
         if (write_ret < buf_len) {
            mongoc_counter_streams_timeout_inc ();
         }

         tls->timeout_msec = 0;
      } else {
         tls->timeout_msec = (expire - now) / 1000L;
      }
   }

   RETURN (write_ret);
}

/* This is copypasta from _mongoc_stream_tls_openssl_writev */
#define MONGOC_STREAM_TLS_BUFFER_SIZE 4096
static ssize_t
_mongoc_stream_tls_secure_transport_writev (mongoc_stream_t *stream,
                                            mongoc_iovec_t *iov,
                                            size_t iovcnt,
                                            int32_t timeout_msec)
{
   mongoc_stream_tls_t *tls = (mongoc_stream_tls_t *) stream;
   mongoc_stream_tls_secure_transport_t *secure_transport =
      (mongoc_stream_tls_secure_transport_t *) tls->ctx;
   char buf[MONGOC_STREAM_TLS_BUFFER_SIZE];
   ssize_t ret = 0;
   ssize_t child_ret;
   size_t i;
   size_t iov_pos = 0;

   /* There's a bit of a dance to coalesce vectorized writes into
    * MONGOC_STREAM_TLS_BUFFER_SIZE'd writes to avoid lots of small tls
    * packets.
    *
    * The basic idea is that we want to combine writes in the buffer if they're
    * smaller than the buffer, flushing as it gets full.  For larger writes, or
    * the last write in the iovec array, we want to ignore the buffer and just
    * write immediately.  We take care of doing buffer writes by re-invoking
    * ourself with a single iovec_t, pointing at our stack buffer.
    */
   char *buf_head = buf;
   char *buf_tail = buf;
   char *buf_end = buf + MONGOC_STREAM_TLS_BUFFER_SIZE;
   size_t bytes;

   char *to_write = NULL;
   size_t to_write_len;

   BSON_ASSERT (iov);
   BSON_ASSERT (iovcnt);
   BSON_ASSERT (secure_transport);
   ENTRY;

   tls->timeout_msec = timeout_msec;

   for (i = 0; i < iovcnt; i++) {
      iov_pos = 0;

      while (iov_pos < iov[i].iov_len) {
         if (buf_head != buf_tail ||
             ((i + 1 < iovcnt) &&
              ((buf_end - buf_tail) > (iov[i].iov_len - iov_pos)))) {
            /* If we have either of:
             *   - buffered bytes already
             *   - another iovec to send after this one and we don't have more
             *     bytes to send than the size of the buffer.
             *
             * copy into the buffer */

            bytes = BSON_MIN (iov[i].iov_len - iov_pos, buf_end - buf_tail);

            memcpy (buf_tail, (char *) iov[i].iov_base + iov_pos, bytes);
            buf_tail += bytes;
            iov_pos += bytes;

            if (buf_tail == buf_end) {
               /* If we're full, request send */

               to_write = buf_head;
               to_write_len = buf_tail - buf_head;

               buf_tail = buf_head = buf;
            }
         } else {
            /* Didn't buffer, so just write it through */

            to_write = (char *) iov[i].iov_base + iov_pos;
            to_write_len = iov[i].iov_len - iov_pos;

            iov_pos += to_write_len;
         }

         if (to_write) {
            /* We get here if we buffered some bytes and filled the buffer, or
             * if we didn't buffer and have to send out of the iovec */

            child_ret = _mongoc_stream_tls_secure_transport_write (
               stream, to_write, to_write_len);

            if (child_ret < 0) {
               RETURN (ret);
            }

            ret += child_ret;

            if (child_ret < to_write_len) {
               /* we timed out, so send back what we could send */

               RETURN (ret);
            }

            to_write = NULL;
         }
      }
   }

   if (buf_head != buf_tail) {
      /* If we have any bytes buffered, send */

      child_ret = _mongoc_stream_tls_secure_transport_write (
         stream, buf_head, buf_tail - buf_head);

      if (child_ret < 0) {
         RETURN (child_ret);
      }

      ret += child_ret;
   }

   if (ret >= 0) {
      mongoc_counter_streams_egress_add (ret);
   }

   TRACE ("Returning %zu", ret);
   RETURN (ret);
}

/* This function is copypasta of _mongoc_stream_tls_openssl_readv */
static ssize_t
_mongoc_stream_tls_secure_transport_readv (mongoc_stream_t *stream,
                                           mongoc_iovec_t *iov,
                                           size_t iovcnt,
                                           size_t min_bytes,
                                           int32_t timeout_msec)
{
   mongoc_stream_tls_t *tls = (mongoc_stream_tls_t *) stream;
   mongoc_stream_tls_secure_transport_t *secure_transport =
      (mongoc_stream_tls_secure_transport_t *) tls->ctx;
   ssize_t ret = 0;
   size_t i;
   size_t read_ret;
   size_t iov_pos = 0;
   int64_t now;
   int64_t expire = 0;

   BSON_ASSERT (iov);
   BSON_ASSERT (iovcnt);
   BSON_ASSERT (secure_transport);
   ENTRY;

   tls->timeout_msec = timeout_msec;

   if (timeout_msec >= 0) {
      expire = bson_get_monotonic_time () + (timeout_msec * 1000UL);
   }

   for (i = 0; i < iovcnt; i++) {
      iov_pos = 0;

      while (iov_pos < iov[i].iov_len) {
         OSStatus status = SSLRead (secure_transport->ssl_ctx_ref,
                                    (char *) iov[i].iov_base + iov_pos,
                                    (int) (iov[i].iov_len - iov_pos),
                                    &read_ret);

         if (status != noErr) {
            RETURN (-1);
         }

         if (expire) {
            now = bson_get_monotonic_time ();

            if ((expire - now) < 0) {
               if (read_ret == 0) {
                  mongoc_counter_streams_timeout_inc ();
                  errno = ETIMEDOUT;
                  RETURN (-1);
               }

               tls->timeout_msec = 0;
            } else {
               tls->timeout_msec = (expire - now) / 1000L;
            }
         }

         ret += read_ret;

         if ((size_t) ret >= min_bytes) {
            mongoc_counter_streams_ingress_add (ret);
            RETURN (ret);
         }

         iov_pos += read_ret;
      }
   }

   if (ret >= 0) {
      mongoc_counter_streams_ingress_add (ret);
   }

   RETURN (ret);
}

static int
_mongoc_stream_tls_secure_transport_setsockopt (mongoc_stream_t *stream,
                                                int level,
                                                int optname,
                                                void *optval,
                                                mongoc_socklen_t optlen)
{
   mongoc_stream_tls_t *tls = (mongoc_stream_tls_t *) stream;
   mongoc_stream_tls_secure_transport_t *secure_transport =
      (mongoc_stream_tls_secure_transport_t *) tls->ctx;

   ENTRY;
   BSON_ASSERT (secure_transport);
   RETURN (mongoc_stream_setsockopt (
      tls->base_stream, level, optname, optval, optlen));
}

static mongoc_stream_t *
_mongoc_stream_tls_secure_transport_get_base_stream (mongoc_stream_t *stream)
{
   mongoc_stream_tls_t *tls = (mongoc_stream_tls_t *) stream;
   mongoc_stream_tls_secure_transport_t *secure_transport =
      (mongoc_stream_tls_secure_transport_t *) tls->ctx;

   ENTRY;
   BSON_ASSERT (secure_transport);
   RETURN (tls->base_stream);
}


static bool
_mongoc_stream_tls_secure_transport_check_closed (
   mongoc_stream_t *stream) /* IN */
{
   mongoc_stream_tls_t *tls = (mongoc_stream_tls_t *) stream;
   mongoc_stream_tls_secure_transport_t *secure_transport =
      (mongoc_stream_tls_secure_transport_t *) tls->ctx;

   ENTRY;
   BSON_ASSERT (secure_transport);
   RETURN (mongoc_stream_check_closed (tls->base_stream));
}

bool
mongoc_stream_tls_secure_transport_handshake (mongoc_stream_t *stream,
                                              const char *host,
                                              int *events,
                                              bson_error_t *error)
{
   OSStatus ret = 0;
   mongoc_stream_tls_t *tls = (mongoc_stream_tls_t *) stream;
   mongoc_stream_tls_secure_transport_t *secure_transport =
      (mongoc_stream_tls_secure_transport_t *) tls->ctx;

   ENTRY;
   BSON_ASSERT (secure_transport);

   ret = SSLHandshake (secure_transport->ssl_ctx_ref);
   /* Weak certificate validation requested, eg: none */

   if (ret == errSSLServerAuthCompleted) {
      ret = errSSLWouldBlock;
   }

   if (ret == noErr) {
      RETURN (true);
   }

   if (ret == errSSLWouldBlock) {
      *events = POLLIN | POLLOUT;
   } else {
      *events = 0;
      bson_set_error (error,
                      MONGOC_ERROR_STREAM,
                      MONGOC_ERROR_STREAM_SOCKET,
                      "TLS handshake failed: %d",
                      ret);
   }

   RETURN (false);
}

mongoc_stream_t *
mongoc_stream_tls_secure_transport_new (mongoc_stream_t *base_stream,
                                        const char *host,
                                        mongoc_ssl_opt_t *opt,
                                        int client)
{
   mongoc_stream_tls_t *tls;
   mongoc_stream_tls_secure_transport_t *secure_transport;

   ENTRY;
   BSON_ASSERT (base_stream);
   BSON_ASSERT (opt);

   if (opt->ca_dir) {
      MONGOC_ERROR ("Setting mongoc_ssl_opt_t.ca_dir has no effect when built "
                    "against Secure Transport");
      RETURN (false);
   }
   if (opt->crl_file) {
      MONGOC_ERROR (
         "Setting mongoc_ssl_opt_t.crl_file has no effect when built "
         "against Secure Transport");
      RETURN (false);
   }


   secure_transport = (mongoc_stream_tls_secure_transport_t *) bson_malloc0 (
      sizeof *secure_transport);

   tls = (mongoc_stream_tls_t *) bson_malloc0 (sizeof *tls);
   tls->parent.type = MONGOC_STREAM_TLS;
   tls->parent.destroy = _mongoc_stream_tls_secure_transport_destroy;
   tls->parent.failed = _mongoc_stream_tls_secure_transport_failed;
   tls->parent.close = _mongoc_stream_tls_secure_transport_close;
   tls->parent.flush = _mongoc_stream_tls_secure_transport_flush;
   tls->parent.writev = _mongoc_stream_tls_secure_transport_writev;
   tls->parent.readv = _mongoc_stream_tls_secure_transport_readv;
   tls->parent.setsockopt = _mongoc_stream_tls_secure_transport_setsockopt;
   tls->parent.get_base_stream =
      _mongoc_stream_tls_secure_transport_get_base_stream;
   tls->parent.check_closed = _mongoc_stream_tls_secure_transport_check_closed;
   memcpy (&tls->ssl_opts, opt, sizeof tls->ssl_opts);
   tls->handshake = mongoc_stream_tls_secure_transport_handshake;
   tls->ctx = (void *) secure_transport;
   tls->timeout_msec = -1;
   tls->base_stream = base_stream;

   secure_transport->ssl_ctx_ref =
      SSLCreateContext (kCFAllocatorDefault,
                        client ? kSSLClientSide : kSSLServerSide,
                        kSSLStreamType);

   SSLSetIOFuncs (secure_transport->ssl_ctx_ref,
                  mongoc_secure_transport_read,
                  mongoc_secure_transport_write);
   SSLSetProtocolVersionMin (secure_transport->ssl_ctx_ref, kTLSProtocol1);

   mongoc_secure_transport_setup_certificate (secure_transport, opt);
   mongoc_secure_transport_setup_ca (secure_transport, opt);

   if (client) {
      SSLSetSessionOption (secure_transport->ssl_ctx_ref,
                           kSSLSessionOptionBreakOnServerAuth,
                           opt->weak_cert_validation);
   } else if (!opt->allow_invalid_hostname) {
      /* used only in mock_server_t tests */
      SSLSetClientSideAuthenticate (secure_transport->ssl_ctx_ref,
                                    kAlwaysAuthenticate);
   }

   if (!opt->allow_invalid_hostname) {
      SSLSetPeerDomainName (secure_transport->ssl_ctx_ref, host, strlen (host));
   }
   SSLSetConnection (secure_transport->ssl_ctx_ref, tls);


   mongoc_counter_streams_active_inc ();
   RETURN ((mongoc_stream_t *) tls);
}
#endif /* MONGOC_ENABLE_SSL_SECURE_TRANSPORT */
