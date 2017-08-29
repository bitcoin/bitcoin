/*
 * Copyright 2014 MongoDB, Inc.
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


#include "mongoc-stream-private.h"
#include "mongoc-stream-socket.h"
#include "mongoc-trace-private.h"


#undef MONGOC_LOG_DOMAIN
#define MONGOC_LOG_DOMAIN "stream"


struct _mongoc_stream_socket_t {
   mongoc_stream_t vtable;
   mongoc_socket_t *sock;
};


static BSON_INLINE int64_t
get_expiration (int32_t timeout_msec)
{
   if (timeout_msec < 0) {
      return -1;
   } else if (timeout_msec == 0) {
      return 0;
   } else {
      return (bson_get_monotonic_time () + ((int64_t) timeout_msec * 1000L));
   }
}


static int
_mongoc_stream_socket_close (mongoc_stream_t *stream)
{
   mongoc_stream_socket_t *ss = (mongoc_stream_socket_t *) stream;
   int ret;

   ENTRY;

   BSON_ASSERT (ss);

   if (ss->sock) {
      ret = mongoc_socket_close (ss->sock);
      RETURN (ret);
   }

   RETURN (0);
}


static void
_mongoc_stream_socket_destroy (mongoc_stream_t *stream)
{
   mongoc_stream_socket_t *ss = (mongoc_stream_socket_t *) stream;

   ENTRY;

   BSON_ASSERT (ss);

   if (ss->sock) {
      mongoc_socket_destroy (ss->sock);
      ss->sock = NULL;
   }

   bson_free (ss);

   EXIT;
}


static void
_mongoc_stream_socket_failed (mongoc_stream_t *stream)
{
   ENTRY;

   _mongoc_stream_socket_destroy (stream);

   EXIT;
}


static int
_mongoc_stream_socket_setsockopt (mongoc_stream_t *stream,
                                  int level,
                                  int optname,
                                  void *optval,
                                  mongoc_socklen_t optlen)
{
   mongoc_stream_socket_t *ss = (mongoc_stream_socket_t *) stream;
   int ret;

   ENTRY;

   BSON_ASSERT (ss);
   BSON_ASSERT (ss->sock);

   ret = mongoc_socket_setsockopt (ss->sock, level, optname, optval, optlen);

   RETURN (ret);
}


static int
_mongoc_stream_socket_flush (mongoc_stream_t *stream)
{
   ENTRY;
   RETURN (0);
}


static ssize_t
_mongoc_stream_socket_readv (mongoc_stream_t *stream,
                             mongoc_iovec_t *iov,
                             size_t iovcnt,
                             size_t min_bytes,
                             int32_t timeout_msec)
{
   mongoc_stream_socket_t *ss = (mongoc_stream_socket_t *) stream;
   int64_t expire_at;
   ssize_t ret = 0;
   ssize_t nread;
   size_t cur = 0;

   ENTRY;

   BSON_ASSERT (ss);
   BSON_ASSERT (ss->sock);

   expire_at = get_expiration (timeout_msec);

   /*
    * This isn't ideal, we should plumb through to recvmsg(), but we
    * don't actually use this in any way but to a single buffer
    * currently anyway, so should be just fine.
    */

   for (;;) {
      nread = mongoc_socket_recv (
         ss->sock, iov[cur].iov_base, iov[cur].iov_len, 0, expire_at);

      if (nread <= 0) {
         if (ret >= (ssize_t) min_bytes) {
            RETURN (ret);
         }
         errno = mongoc_socket_errno (ss->sock);
         RETURN (-1);
      }

      ret += nread;

      while ((cur < iovcnt) && (nread >= (ssize_t) iov[cur].iov_len)) {
         nread -= iov[cur++].iov_len;
      }

      if (cur == iovcnt) {
         break;
      }

      if (ret >= (ssize_t) min_bytes) {
         RETURN (ret);
      }

      iov[cur].iov_base = ((char *) iov[cur].iov_base) + nread;
      iov[cur].iov_len -= nread;

      BSON_ASSERT (iovcnt - cur);
      BSON_ASSERT (iov[cur].iov_len);
   }

   RETURN (ret);
}


static ssize_t
_mongoc_stream_socket_writev (mongoc_stream_t *stream,
                              mongoc_iovec_t *iov,
                              size_t iovcnt,
                              int32_t timeout_msec)
{
   mongoc_stream_socket_t *ss = (mongoc_stream_socket_t *) stream;
   int64_t expire_at;
   ssize_t ret;

   ENTRY;

   if (ss->sock) {
      expire_at = get_expiration (timeout_msec);
      ret = mongoc_socket_sendv (ss->sock, iov, iovcnt, expire_at);
      errno = mongoc_socket_errno (ss->sock);
      RETURN (ret);
   }

   RETURN (-1);
}


static ssize_t
_mongoc_stream_socket_poll (mongoc_stream_poll_t *streams,
                            size_t nstreams,
                            int32_t timeout_msec)

{
   int i;
   ssize_t ret = -1;
   mongoc_socket_poll_t *sds;
   mongoc_stream_socket_t *ss;

   ENTRY;

   sds = (mongoc_socket_poll_t *) bson_malloc (sizeof (*sds) * nstreams);

   for (i = 0; i < nstreams; i++) {
      ss = (mongoc_stream_socket_t *) streams[i].stream;

      if (!ss->sock) {
         goto CLEANUP;
      }

      sds[i].socket = ss->sock;
      sds[i].events = streams[i].events;
   }

   ret = mongoc_socket_poll (sds, nstreams, timeout_msec);

   if (ret > 0) {
      for (i = 0; i < nstreams; i++) {
         streams[i].revents = sds[i].revents;
      }
   }

CLEANUP:
   bson_free (sds);

   RETURN (ret);
}


mongoc_socket_t *
mongoc_stream_socket_get_socket (mongoc_stream_socket_t *stream) /* IN */
{
   BSON_ASSERT (stream);

   return stream->sock;
}


static bool
_mongoc_stream_socket_check_closed (mongoc_stream_t *stream) /* IN */
{
   mongoc_stream_socket_t *ss = (mongoc_stream_socket_t *) stream;

   ENTRY;

   BSON_ASSERT (stream);

   if (ss->sock) {
      RETURN (mongoc_socket_check_closed (ss->sock));
   }

   RETURN (true);
}


/*
 *--------------------------------------------------------------------------
 *
 * mongoc_stream_socket_new --
 *
 *       Create a new mongoc_stream_t using the mongoc_socket_t for
 *       read and write underneath.
 *
 * Returns:
 *       None.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

mongoc_stream_t *
mongoc_stream_socket_new (mongoc_socket_t *sock) /* IN */
{
   mongoc_stream_socket_t *stream;

   BSON_ASSERT (sock);

   stream = (mongoc_stream_socket_t *) bson_malloc0 (sizeof *stream);
   stream->vtable.type = MONGOC_STREAM_SOCKET;
   stream->vtable.close = _mongoc_stream_socket_close;
   stream->vtable.destroy = _mongoc_stream_socket_destroy;
   stream->vtable.failed = _mongoc_stream_socket_failed;
   stream->vtable.flush = _mongoc_stream_socket_flush;
   stream->vtable.readv = _mongoc_stream_socket_readv;
   stream->vtable.writev = _mongoc_stream_socket_writev;
   stream->vtable.setsockopt = _mongoc_stream_socket_setsockopt;
   stream->vtable.check_closed = _mongoc_stream_socket_check_closed;
   stream->vtable.poll = _mongoc_stream_socket_poll;
   stream->sock = sock;

   return (mongoc_stream_t *) stream;
}
