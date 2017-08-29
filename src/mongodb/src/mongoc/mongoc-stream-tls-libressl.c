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

#ifdef MONGOC_ENABLE_SSL_LIBRESSL

#include <bson.h>

#include "mongoc-trace-private.h"
#include "mongoc-log.h"
#include "mongoc-stream-tls.h"
#include "mongoc-stream-tls-private.h"
#include "mongoc-stream-private.h"
#include "mongoc-stream-tls-libressl-private.h"
#include "mongoc-libressl-private.h"
#include "mongoc-ssl.h"
#include "mongoc-error.h"
#include "mongoc-counters-private.h"
#include "mongoc-stream-socket.h"
#include "mongoc-socket-private.h"

#include <tls.h>

#undef MONGOC_LOG_DOMAIN
#define MONGOC_LOG_DOMAIN "stream-tls-libressl"

static void
_mongoc_stream_tls_libressl_destroy (mongoc_stream_t *stream)
{
   mongoc_stream_tls_t *tls = (mongoc_stream_tls_t *) stream;
   mongoc_stream_tls_libressl_t *libressl =
      (mongoc_stream_tls_libressl_t *) tls->ctx;

   ENTRY;
   BSON_ASSERT (libressl);

   tls_close (libressl->ctx);
   tls_free (libressl->ctx);
   tls_config_free (libressl->config);

   mongoc_stream_destroy (tls->base_stream);

   bson_free (libressl);
   bson_free (stream);

   mongoc_counter_streams_active_dec ();
   mongoc_counter_streams_disposed_inc ();
   EXIT;
}

static void
_mongoc_stream_tls_libressl_failed (mongoc_stream_t *stream)
{
   ENTRY;
   _mongoc_stream_tls_libressl_destroy (stream);
   EXIT;
}

static int
_mongoc_stream_tls_libressl_close (mongoc_stream_t *stream)
{
   int ret = 0;
   mongoc_stream_tls_t *tls = (mongoc_stream_tls_t *) stream;
   mongoc_stream_tls_libressl_t *libressl =
      (mongoc_stream_tls_libressl_t *) tls->ctx;

   ENTRY;
   BSON_ASSERT (libressl);

   ret = mongoc_stream_close (tls->base_stream);
   RETURN (ret);
}

static int
_mongoc_stream_tls_libressl_flush (mongoc_stream_t *stream)
{
   mongoc_stream_tls_t *tls = (mongoc_stream_tls_t *) stream;
   mongoc_stream_tls_libressl_t *libressl =
      (mongoc_stream_tls_libressl_t *) tls->ctx;

   ENTRY;
   BSON_ASSERT (libressl);
   RETURN (0);
}

static ssize_t
_mongoc_stream_tls_libressl_write (mongoc_stream_t *stream,
                                   char *buf,
                                   size_t buf_len)
{
   mongoc_stream_tls_t *tls = (mongoc_stream_tls_t *) stream;
   mongoc_stream_tls_libressl_t *libressl =
      (mongoc_stream_tls_libressl_t *) tls->ctx;
   mongoc_stream_poll_t poller;
   ssize_t total_write = 0;
   ssize_t ret;
   int64_t now;
   int64_t expire = 0;

   ENTRY;
   BSON_ASSERT (libressl);

   if (tls->timeout_msec >= 0) {
      expire = bson_get_monotonic_time () + (tls->timeout_msec * 1000UL);
   }

   do {
      poller.stream = stream;
      poller.revents = 0;
      poller.events = POLLOUT;
      ret = tls_write (libressl->ctx, buf, buf_len);

      if (ret == TLS_WANT_POLLIN) {
         poller.events = POLLIN;
         mongoc_stream_poll (&poller, 1, tls->timeout_msec);
      } else if (ret == TLS_WANT_POLLOUT) {
         poller.events = POLLOUT;
         mongoc_stream_poll (&poller, 1, tls->timeout_msec);
      } else if (ret < 0) {
         RETURN (total_write);
      } else {
         buf += ret;
         buf_len -= ret;
         total_write += ret;
      }
      if (expire) {
         now = bson_get_monotonic_time ();

         if ((expire - now) < 0) {
            if (ret == 0) {
               mongoc_counter_streams_timeout_inc ();
               break;
            }

            tls->timeout_msec = 0;
         } else {
            tls->timeout_msec = (expire - now) / 1000L;
         }
      }
   } while (buf_len > 0);


   RETURN (total_write);
}

/* This is copypasta from _mongoc_stream_tls_openssl_writev */
#define MONGOC_STREAM_TLS_BUFFER_SIZE 4096
static ssize_t
_mongoc_stream_tls_libressl_writev (mongoc_stream_t *stream,
                                    mongoc_iovec_t *iov,
                                    size_t iovcnt,
                                    int32_t timeout_msec)
{
   mongoc_stream_tls_t *tls = (mongoc_stream_tls_t *) stream;
   mongoc_stream_tls_libressl_t *libressl =
      (mongoc_stream_tls_libressl_t *) tls->ctx;
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
   BSON_ASSERT (libressl);
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

            child_ret = _mongoc_stream_tls_libressl_write (
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

      child_ret = _mongoc_stream_tls_libressl_write (
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
_mongoc_stream_tls_libressl_readv (mongoc_stream_t *stream,
                                   mongoc_iovec_t *iov,
                                   size_t iovcnt,
                                   size_t min_bytes,
                                   int32_t timeout_msec)
{
   mongoc_stream_tls_t *tls = (mongoc_stream_tls_t *) stream;
   mongoc_stream_tls_libressl_t *libressl =
      (mongoc_stream_tls_libressl_t *) tls->ctx;
   ssize_t ret = 0;
   ssize_t read_ret;
   size_t i;
   size_t iov_pos = 0;
   int64_t now;
   int64_t expire = 0;
   mongoc_stream_poll_t poller;

   BSON_ASSERT (iov);
   BSON_ASSERT (iovcnt);
   BSON_ASSERT (libressl);
   ENTRY;

   tls->timeout_msec = timeout_msec;

   if (timeout_msec >= 0) {
      expire = bson_get_monotonic_time () + (timeout_msec * 1000UL);
   }

   for (i = 0; i < iovcnt; i++) {
      iov_pos = 0;

      while (iov_pos < iov[i].iov_len) {
         poller.stream = stream;
         poller.revents = 0;
         poller.events = POLLIN;
         read_ret = tls_read (libressl->ctx,
                              (char *) iov[i].iov_base + iov_pos,
                              (int) (iov[i].iov_len - iov_pos));

         if (read_ret == TLS_WANT_POLLIN) {
            poller.events = POLLIN;
            mongoc_stream_poll (&poller, 1, tls->timeout_msec);
         } else if (read_ret == TLS_WANT_POLLOUT) {
            poller.events = POLLOUT;
            mongoc_stream_poll (&poller, 1, tls->timeout_msec);
         } else if (read_ret < 0) {
            RETURN (ret);
         } else {
            iov_pos += read_ret;
            ret += read_ret;
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


         if (ret > 0 && (size_t) ret >= min_bytes) {
            mongoc_counter_streams_ingress_add (ret);
            RETURN (ret);
         }
      }
   }

   if (ret >= 0) {
      mongoc_counter_streams_ingress_add (ret);
   }

   RETURN (ret);
}

static int
_mongoc_stream_tls_libressl_setsockopt (mongoc_stream_t *stream,
                                        int level,
                                        int optname,
                                        void *optval,
                                        mongoc_socklen_t optlen)
{
   mongoc_stream_tls_t *tls = (mongoc_stream_tls_t *) stream;
   mongoc_stream_tls_libressl_t *libressl =
      (mongoc_stream_tls_libressl_t *) tls->ctx;

   ENTRY;
   BSON_ASSERT (libressl);
   RETURN (mongoc_stream_setsockopt (
      tls->base_stream, level, optname, optval, optlen));
}

static mongoc_stream_t *
_mongoc_stream_tls_libressl_get_base_stream (mongoc_stream_t *stream)
{
   mongoc_stream_tls_t *tls = (mongoc_stream_tls_t *) stream;
   mongoc_stream_tls_libressl_t *libressl =
      (mongoc_stream_tls_libressl_t *) tls->ctx;

   ENTRY;
   BSON_ASSERT (libressl);
   RETURN (tls->base_stream);
}


static bool
_mongoc_stream_tls_libressl_check_closed (mongoc_stream_t *stream)
{
   mongoc_stream_tls_t *tls = (mongoc_stream_tls_t *) stream;
   mongoc_stream_tls_libressl_t *libressl =
      (mongoc_stream_tls_libressl_t *) tls->ctx;

   ENTRY;
   BSON_ASSERT (libressl);
   RETURN (mongoc_stream_check_closed (tls->base_stream));
}

bool
mongoc_stream_tls_libressl_handshake (mongoc_stream_t *stream,
                                      const char *host,
                                      int *events,
                                      bson_error_t *error)
{
   mongoc_stream_tls_t *tls = (mongoc_stream_tls_t *) stream;
   mongoc_stream_tls_libressl_t *libressl =
      (mongoc_stream_tls_libressl_t *) tls->ctx;
   int ret;

   ENTRY;
   BSON_ASSERT (libressl);

   ret = tls_handshake (libressl->ctx);

   if (ret == TLS_WANT_POLLIN) {
      *events = POLLIN;
   } else if (ret == TLS_WANT_POLLOUT) {
      *events = POLLOUT;
   } else if (ret < 0) {
      *events = 0;
      bson_set_error (error,
                      MONGOC_ERROR_STREAM,
                      MONGOC_ERROR_STREAM_SOCKET,
                      "TLS handshake failed: %s",
                      tls_error (libressl->ctx));
      RETURN (false);
   } else {
      RETURN (true);
   }
   RETURN (false);
}

mongoc_stream_t *
mongoc_stream_tls_libressl_new (mongoc_stream_t *base_stream,
                                const char *host,
                                mongoc_ssl_opt_t *opt,
                                int client)
{
   mongoc_stream_tls_t *tls;
   mongoc_stream_tls_libressl_t *libressl;

   ENTRY;
   BSON_ASSERT (base_stream);
   BSON_ASSERT (opt);


   if (opt->crl_file) {
      MONGOC_ERROR (
         "Setting mongoc_ssl_opt_t.crl_file has no effect when built "
         "against libtls");
      RETURN (false);
   }
   libressl = (mongoc_stream_tls_libressl_t *) bson_malloc0 (sizeof *libressl);

   tls = (mongoc_stream_tls_t *) bson_malloc0 (sizeof *tls);
   tls->parent.type = MONGOC_STREAM_TLS;
   tls->parent.destroy = _mongoc_stream_tls_libressl_destroy;
   tls->parent.failed = _mongoc_stream_tls_libressl_failed;
   tls->parent.close = _mongoc_stream_tls_libressl_close;
   tls->parent.flush = _mongoc_stream_tls_libressl_flush;
   tls->parent.writev = _mongoc_stream_tls_libressl_writev;
   tls->parent.readv = _mongoc_stream_tls_libressl_readv;
   tls->parent.setsockopt = _mongoc_stream_tls_libressl_setsockopt;
   tls->parent.get_base_stream = _mongoc_stream_tls_libressl_get_base_stream;
   tls->parent.check_closed = _mongoc_stream_tls_libressl_check_closed;
   memcpy (&tls->ssl_opts, opt, sizeof tls->ssl_opts);
   tls->handshake = mongoc_stream_tls_libressl_handshake;
   tls->ctx = (void *) libressl;
   tls->timeout_msec = -1;
   tls->base_stream = base_stream;

   libressl->ctx = client ? tls_client () : tls_server ();
   libressl->config = tls_config_new ();

   if (opt->weak_cert_validation) {
      tls_config_insecure_noverifycert (libressl->config);
      tls_config_insecure_noverifytime (libressl->config);
   }
   if (opt->allow_invalid_hostname) {
      tls_config_insecure_noverifyname (libressl->config);
   }
   tls_config_set_ciphers (libressl->config, "compat");

   mongoc_libressl_setup_certificate (libressl, opt);
   mongoc_libressl_setup_ca (libressl, opt);
   {
      mongoc_stream_t *stream = base_stream;

      do {
         if (stream->type == MONGOC_STREAM_SOCKET) {
            int socket = mongoc_stream_socket_get_socket (
                            (mongoc_stream_socket_t *) stream)
                            ->sd;
            if (tls_configure (libressl->ctx, libressl->config) == -1) {
               MONGOC_ERROR ("%s", tls_config_error (libressl->config));
               RETURN (false);
            }
            if (tls_connect_socket (libressl->ctx, socket, host) == -1) {
               MONGOC_ERROR ("%s", tls_error (libressl->ctx));
               RETURN (false);
            }
            break;
         }
      } while ((stream = mongoc_stream_get_base_stream (stream)));
   }

   mongoc_counter_streams_active_inc ();
   RETURN ((mongoc_stream_t *) tls);
}
#endif /* MONGOC_ENABLE_SSL_LIBRESSL */
