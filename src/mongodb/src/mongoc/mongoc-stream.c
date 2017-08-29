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

#include "mongoc-array-private.h"
#include "mongoc-buffer-private.h"
#include "mongoc-error.h"
#include "mongoc-errno-private.h"
#include "mongoc-flags.h"
#include "mongoc-log.h"
#include "mongoc-opcode.h"
#include "mongoc-rpc-private.h"
#include "mongoc-stream.h"
#include "mongoc-stream-private.h"
#include "mongoc-trace-private.h"
#include "mongoc-util-private.h"


#undef MONGOC_LOG_DOMAIN
#define MONGOC_LOG_DOMAIN "stream"

#ifndef MONGOC_DEFAULT_TIMEOUT_MSEC
#define MONGOC_DEFAULT_TIMEOUT_MSEC (60L * 60L * 1000L)
#endif


/**
 * mongoc_stream_close:
 * @stream: A mongoc_stream_t.
 *
 * Closes the underlying file-descriptor used by @stream.
 *
 * Returns: 0 on success, -1 on failure.
 */
int
mongoc_stream_close (mongoc_stream_t *stream)
{
   int ret;

   ENTRY;

   BSON_ASSERT (stream);

   BSON_ASSERT (stream->close);

   ret = stream->close (stream);

   RETURN (ret);
}


/**
 * mongoc_stream_failed:
 * @stream: A mongoc_stream_t.
 *
 * Frees any resources referenced by @stream, including the memory allocation
 * for @stream.
 * This handler is called upon stream failure, such as network errors, invalid
 * replies
 * or replicaset reconfigures deleteing the stream
 */
void
mongoc_stream_failed (mongoc_stream_t *stream)
{
   ENTRY;

   BSON_ASSERT (stream);

   if (stream->failed) {
      stream->failed (stream);
   } else {
      stream->destroy (stream);
   }

   EXIT;
}


/**
 * mongoc_stream_destroy:
 * @stream: A mongoc_stream_t.
 *
 * Frees any resources referenced by @stream, including the memory allocation
 * for @stream.
 */
void
mongoc_stream_destroy (mongoc_stream_t *stream)
{
   ENTRY;

   BSON_ASSERT (stream);

   BSON_ASSERT (stream->destroy);

   stream->destroy (stream);

   EXIT;
}


/**
 * mongoc_stream_flush:
 * @stream: A mongoc_stream_t.
 *
 * Flushes the data in the underlying stream to the transport.
 *
 * Returns: 0 on success, -1 on failure.
 */
int
mongoc_stream_flush (mongoc_stream_t *stream)
{
   BSON_ASSERT (stream);
   return stream->flush (stream);
}


/**
 * mongoc_stream_writev:
 * @stream: A mongoc_stream_t.
 * @iov: An array of iovec to write to the stream.
 * @iovcnt: The number of elements in @iov.
 *
 * Writes an array of iovec buffers to the underlying stream.
 *
 * Returns: the number of bytes written, or -1 upon failure.
 */
ssize_t
mongoc_stream_writev (mongoc_stream_t *stream,
                      mongoc_iovec_t *iov,
                      size_t iovcnt,
                      int32_t timeout_msec)
{
   ssize_t ret;

   ENTRY;

   BSON_ASSERT (stream);
   BSON_ASSERT (iov);
   BSON_ASSERT (iovcnt);

   BSON_ASSERT (stream->writev);

   if (timeout_msec < 0) {
      timeout_msec = MONGOC_DEFAULT_TIMEOUT_MSEC;
   }

   DUMP_IOVEC (writev, iov, iovcnt);
   ret = stream->writev (stream, iov, iovcnt, timeout_msec);

   RETURN (ret);
}

/**
 * mongoc_stream_write:
 * @stream: A mongoc_stream_t.
 * @buf: A buffer to write.
 * @count: The number of bytes to write into @buf.
 *
 * Simplified access to mongoc_stream_writev(). Creates a single iovec
 * with the buffer provided.
 *
 * Returns: -1 on failure, otherwise the number of bytes write.
 */
ssize_t
mongoc_stream_write (mongoc_stream_t *stream,
                     void *buf,
                     size_t count,
                     int32_t timeout_msec)
{
   mongoc_iovec_t iov;
   ssize_t ret;

   ENTRY;

   BSON_ASSERT (stream);
   BSON_ASSERT (buf);

   iov.iov_base = buf;
   iov.iov_len = count;

   BSON_ASSERT (stream->writev);

   ret = mongoc_stream_writev (stream, &iov, 1, timeout_msec);

   RETURN (ret);
}

/**
 * mongoc_stream_readv:
 * @stream: A mongoc_stream_t.
 * @iov: An array of iovec containing the location and sizes to read.
 * @iovcnt: the number of elements in @iov.
 * @min_bytes: the minumum number of bytes to return, or -1.
 *
 * Reads into the various buffers pointed to by @iov and associated
 * buffer lengths.
 *
 * If @min_bytes is specified, then at least min_bytes will be returned unless
 * eof is encountered.  This may result in ETIMEDOUT
 *
 * Returns: the number of bytes read or -1 on failure.
 */
ssize_t
mongoc_stream_readv (mongoc_stream_t *stream,
                     mongoc_iovec_t *iov,
                     size_t iovcnt,
                     size_t min_bytes,
                     int32_t timeout_msec)
{
   ssize_t ret;

   ENTRY;

   BSON_ASSERT (stream);
   BSON_ASSERT (iov);
   BSON_ASSERT (iovcnt);

   BSON_ASSERT (stream->readv);

   ret = stream->readv (stream, iov, iovcnt, min_bytes, timeout_msec);
   if (ret >= 0) {
      DUMP_IOVEC (readv, iov, iovcnt);
   }

   RETURN (ret);
}


/**
 * mongoc_stream_read:
 * @stream: A mongoc_stream_t.
 * @buf: A buffer to write into.
 * @count: The number of bytes to write into @buf.
 * @min_bytes: The minimum number of bytes to receive
 *
 * Simplified access to mongoc_stream_readv(). Creates a single iovec
 * with the buffer provided.
 *
 * If @min_bytes is specified, then at least min_bytes will be returned unless
 * eof is encountered.  This may result in ETIMEDOUT
 *
 * Returns: -1 on failure, otherwise the number of bytes read.
 */
ssize_t
mongoc_stream_read (mongoc_stream_t *stream,
                    void *buf,
                    size_t count,
                    size_t min_bytes,
                    int32_t timeout_msec)
{
   mongoc_iovec_t iov;
   ssize_t ret;

   ENTRY;

   BSON_ASSERT (stream);
   BSON_ASSERT (buf);

   iov.iov_base = buf;
   iov.iov_len = count;

   BSON_ASSERT (stream->readv);

   ret = mongoc_stream_readv (stream, &iov, 1, min_bytes, timeout_msec);

   RETURN (ret);
}


int
mongoc_stream_setsockopt (mongoc_stream_t *stream,
                          int level,
                          int optname,
                          void *optval,
                          mongoc_socklen_t optlen)
{
   BSON_ASSERT (stream);

   if (stream->setsockopt) {
      return stream->setsockopt (stream, level, optname, optval, optlen);
   }

   return 0;
}


mongoc_stream_t *
mongoc_stream_get_base_stream (mongoc_stream_t *stream) /* IN */
{
   BSON_ASSERT (stream);

   if (stream->get_base_stream) {
      return stream->get_base_stream (stream);
   }

   return stream;
}


static mongoc_stream_t *
mongoc_stream_get_root_stream (mongoc_stream_t *stream)

{
   BSON_ASSERT (stream);

   while (stream->get_base_stream) {
      stream = stream->get_base_stream (stream);
   }

   return stream;
}

mongoc_stream_t *
mongoc_stream_get_tls_stream (mongoc_stream_t *stream) /* IN */
{
   BSON_ASSERT (stream);

   for (; stream && stream->type != MONGOC_STREAM_TLS;
        stream = stream->get_base_stream (stream))
      ;

   return stream;
}

ssize_t
mongoc_stream_poll (mongoc_stream_poll_t *streams,
                    size_t nstreams,
                    int32_t timeout)
{
   mongoc_stream_poll_t *poller =
      (mongoc_stream_poll_t *) bson_malloc (sizeof (*poller) * nstreams);

   int i;
   int last_type = 0;
   ssize_t rval = -1;

   errno = 0;

   for (i = 0; i < nstreams; i++) {
      poller[i].stream = mongoc_stream_get_root_stream (streams[i].stream);
      poller[i].events = streams[i].events;
      poller[i].revents = 0;

      if (i == 0) {
         last_type = poller[i].stream->type;
      } else if (last_type != poller[i].stream->type) {
         errno = EINVAL;
         goto CLEANUP;
      }
   }

   if (!poller[0].stream->poll) {
      errno = EINVAL;
      goto CLEANUP;
   }

   rval = poller[0].stream->poll (poller, nstreams, timeout);

   if (rval > 0) {
      for (i = 0; i < nstreams; i++) {
         streams[i].revents = poller[i].revents;
      }
   }

CLEANUP:
   bson_free (poller);

   return rval;
}


/*
 *--------------------------------------------------------------------------
 *
 * mongoc_stream_wait --
 *
 *       Internal helper, poll a single stream until it connects.
 *
 *       For now, only the POLLOUT (connected) event is supported.
 *
 *       @expire_at should be an absolute time at which to expire using
 *       the monotonic clock (bson_get_monotonic_time(), which is in
 *       microseconds). expire_at of 0 or -1 is prohibited.
 *
 * Returns:
 *       true if an event matched. otherwise false.
 *       a timeout will return false.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

bool
mongoc_stream_wait (mongoc_stream_t *stream, int64_t expire_at)
{
   mongoc_stream_poll_t poller;
   int64_t now;
   int32_t timeout_msec;
   ssize_t ret;

   ENTRY;

   BSON_ASSERT (stream);
   BSON_ASSERT (expire_at > 0);

   poller.stream = stream;
   poller.events = POLLOUT;
   poller.revents = 0;

   now = bson_get_monotonic_time ();

   for (;;) {
      /* TODO CDRIVER-804 use int64_t for timeouts consistently */
      timeout_msec = (int32_t) BSON_MIN ((expire_at - now) / 1000L, INT32_MAX);
      if (timeout_msec < 0) {
         timeout_msec = 0;
      }

      ret = mongoc_stream_poll (&poller, 1, timeout_msec);

      if (ret > 0) {
         /* an event happened, return true if POLLOUT else false */
         RETURN (0 != (poller.revents & POLLOUT));
      } else if (ret < 0) {
         /* poll itself failed */

         TRACE ("errno is: %d", errno);
         if (MONGOC_ERRNO_IS_AGAIN (errno)) {
            now = bson_get_monotonic_time ();

            if (expire_at < now) {
               RETURN (false);
            } else {
               continue;
            }
         } else {
            /* poll failed for some non-transient reason */
            RETURN (false);
         }
      } else {
         /* poll timed out */
         RETURN (false);
      }
   }

   return true;
}

bool
mongoc_stream_check_closed (mongoc_stream_t *stream)
{
   int ret;

   ENTRY;

   if (!stream) {
      return true;
   }

   ret = stream->check_closed (stream);

   RETURN (ret);
}

bool
_mongoc_stream_writev_full (mongoc_stream_t *stream,
                            mongoc_iovec_t *iov,
                            size_t iovcnt,
                            int32_t timeout_msec,
                            bson_error_t *error)
{
   size_t total_bytes = 0;
   int i;
   ssize_t r;
   ENTRY;

   for (i = 0; i < iovcnt; i++) {
      total_bytes += iov[i].iov_len;
   }

   r = mongoc_stream_writev (stream, iov, iovcnt, timeout_msec);
   TRACE ("writev returned: %ld", r);

   if (r < 0) {
      if (error) {
         char buf[128];
         char *errstr;

         errstr = bson_strerror_r (errno, buf, sizeof (buf));

         bson_set_error (error,
                         MONGOC_ERROR_STREAM,
                         MONGOC_ERROR_STREAM_SOCKET,
                         "Failure during socket delivery: %s (%d)",
                         errstr,
                         errno);
      }

      RETURN (false);
   }

   if (r != total_bytes) {
      bson_set_error (error,
                      MONGOC_ERROR_STREAM,
                      MONGOC_ERROR_STREAM_SOCKET,
                      "Failure to send all requested bytes (only sent: %" PRIu64
                      "/%" PRId64 " in %dms) during socket delivery",
                      (uint64_t) r,
                      (int64_t) total_bytes,
                      timeout_msec);

      RETURN (false);
   }

   RETURN (true);
}
