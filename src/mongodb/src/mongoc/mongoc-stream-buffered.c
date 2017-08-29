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


#include <errno.h>

#include "mongoc-buffer-private.h"
#include "mongoc-counters-private.h"
#include "mongoc-log.h"
#include "mongoc-stream-buffered.h"
#include "mongoc-stream-private.h"
#include "mongoc-trace-private.h"


#undef MONGOC_LOG_DOMAIN
#define MONGOC_LOG_DOMAIN "stream"


typedef struct {
   mongoc_stream_t stream;
   mongoc_stream_t *base_stream;
   mongoc_buffer_t buffer;
} mongoc_stream_buffered_t;


/*
 *--------------------------------------------------------------------------
 *
 * mongoc_stream_buffered_destroy --
 *
 *       Clean up after a mongoc_stream_buffered_t. Free all allocated
 *       resources and release the base stream.
 *
 * Returns:
 *       None.
 *
 * Side effects:
 *       Everything.
 *
 *--------------------------------------------------------------------------
 */

static void
mongoc_stream_buffered_destroy (mongoc_stream_t *stream) /* IN */
{
   mongoc_stream_buffered_t *buffered = (mongoc_stream_buffered_t *) stream;

   BSON_ASSERT (stream);

   mongoc_stream_destroy (buffered->base_stream);
   buffered->base_stream = NULL;

   _mongoc_buffer_destroy (&buffered->buffer);

   bson_free (stream);

   mongoc_counter_streams_active_dec ();
   mongoc_counter_streams_disposed_inc ();
}


/*
 *--------------------------------------------------------------------------
 *
 * mongoc_stream_buffered_failed --
 *
 *       Called when a stream fails. Useful for streams that differnciate
 *       between failure and cleanup.
 *       Calls mongoc_stream_buffered_destroy() on the stream.
 *
 * Returns:
 *       None.
 *
 * Side effects:
 *       Everything.
 *
 *--------------------------------------------------------------------------
 */

static void
mongoc_stream_buffered_failed (mongoc_stream_t *stream) /* IN */
{
   mongoc_stream_buffered_destroy (stream);
}


/*
 *--------------------------------------------------------------------------
 *
 * mongoc_stream_buffered_close --
 *
 *       Close the underlying stream. The buffered content is still
 *       valid.
 *
 * Returns:
 *       The return value of mongoc_stream_close() on the underlying
 *       stream.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

static int
mongoc_stream_buffered_close (mongoc_stream_t *stream) /* IN */
{
   mongoc_stream_buffered_t *buffered = (mongoc_stream_buffered_t *) stream;
   BSON_ASSERT (stream);
   return mongoc_stream_close (buffered->base_stream);
}


/*
 *--------------------------------------------------------------------------
 *
 * mongoc_stream_buffered_flush --
 *
 *       Flushes the underlying stream.
 *
 * Returns:
 *       The result of flush on the base stream.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

static int
mongoc_stream_buffered_flush (mongoc_stream_t *stream) /* IN */
{
   mongoc_stream_buffered_t *buffered = (mongoc_stream_buffered_t *) stream;
   BSON_ASSERT (buffered);
   return mongoc_stream_flush (buffered->base_stream);
}


/*
 *--------------------------------------------------------------------------
 *
 * mongoc_stream_buffered_writev --
 *
 *       Write an iovec to the underlying stream. This write is not
 *       buffered, it passes through to the base stream directly.
 *
 *       timeout_msec should be the number of milliseconds to wait before
 *       considering the writev as failed.
 *
 * Returns:
 *       The number of bytes written or -1 on failure.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

static ssize_t
mongoc_stream_buffered_writev (mongoc_stream_t *stream, /* IN */
                               mongoc_iovec_t *iov,     /* IN */
                               size_t iovcnt,           /* IN */
                               int32_t timeout_msec)    /* IN */
{
   mongoc_stream_buffered_t *buffered = (mongoc_stream_buffered_t *) stream;
   ssize_t ret;

   ENTRY;

   BSON_ASSERT (buffered);

   ret =
      mongoc_stream_writev (buffered->base_stream, iov, iovcnt, timeout_msec);

   RETURN (ret);
}


/*
 *--------------------------------------------------------------------------
 *
 * mongoc_stream_buffered_readv --
 *
 *       Read from the underlying stream. The data will be buffered based
 *       on the buffered streams target buffer size.
 *
 *       When reading from the underlying stream, we read at least the
 *       requested number of bytes, but try to also fill the stream to
 *       the size of the underlying buffer.
 *
 * Note:
 *       This isn't actually a huge savings since we never have more than
 *       one reply waiting for us, but perhaps someday that will be
 *       different. It should help for small replies, however that will
 *       reduce our read() syscalls by 50%.
 *
 * Returns:
 *       The number of bytes read or -1 on failure.
 *
 * Side effects:
 *       iov[*]->iov_base buffers are filled.
 *
 *--------------------------------------------------------------------------
 */

static ssize_t
mongoc_stream_buffered_readv (mongoc_stream_t *stream, /* IN */
                              mongoc_iovec_t *iov,     /* INOUT */
                              size_t iovcnt,           /* IN */
                              size_t min_bytes,        /* IN */
                              int32_t timeout_msec)    /* IN */
{
   mongoc_stream_buffered_t *buffered = (mongoc_stream_buffered_t *) stream;
   bson_error_t error = {0};
   size_t total_bytes = 0;
   size_t i;

   ENTRY;

   BSON_ASSERT (buffered);

   for (i = 0; i < iovcnt; i++) {
      total_bytes += iov[i].iov_len;
   }

   if (-1 == _mongoc_buffer_fill (&buffered->buffer,
                                  buffered->base_stream,
                                  total_bytes,
                                  timeout_msec,
                                  &error)) {
      MONGOC_WARNING ("%s", error.message);
      RETURN (-1);
   }

   BSON_ASSERT (buffered->buffer.len >= total_bytes);

   for (i = 0; i < iovcnt; i++) {
      memcpy (iov[i].iov_base,
              buffered->buffer.data + buffered->buffer.off,
              iov[i].iov_len);
      buffered->buffer.off += iov[i].iov_len;
      buffered->buffer.len -= iov[i].iov_len;
   }

   RETURN (total_bytes);
}


static mongoc_stream_t *
_mongoc_stream_buffered_get_base_stream (mongoc_stream_t *stream) /* IN */
{
   return ((mongoc_stream_buffered_t *) stream)->base_stream;
}


static bool
_mongoc_stream_buffered_check_closed (mongoc_stream_t *stream) /* IN */
{
   mongoc_stream_buffered_t *buffered = (mongoc_stream_buffered_t *) stream;
   BSON_ASSERT (stream);
   return mongoc_stream_check_closed (buffered->base_stream);
}


/*
 *--------------------------------------------------------------------------
 *
 * mongoc_stream_buffered_new --
 *
 *       Creates a new mongoc_stream_buffered_t.
 *
 *       This stream will read from an underlying stream and try to read
 *       more data than necessary. It can help lower the number of read()
 *       or recv() syscalls performed.
 *
 *       @base_stream is considered owned by the resulting stream after
 *       calling this function.
 *
 * Returns:
 *       A newly allocated mongoc_stream_t.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

mongoc_stream_t *
mongoc_stream_buffered_new (mongoc_stream_t *base_stream, /* IN */
                            size_t buffer_size)           /* IN */
{
   mongoc_stream_buffered_t *stream;

   BSON_ASSERT (base_stream);

   stream = (mongoc_stream_buffered_t *) bson_malloc0 (sizeof *stream);
   stream->stream.type = MONGOC_STREAM_BUFFERED;
   stream->stream.destroy = mongoc_stream_buffered_destroy;
   stream->stream.failed = mongoc_stream_buffered_failed;
   stream->stream.close = mongoc_stream_buffered_close;
   stream->stream.flush = mongoc_stream_buffered_flush;
   stream->stream.writev = mongoc_stream_buffered_writev;
   stream->stream.readv = mongoc_stream_buffered_readv;
   stream->stream.get_base_stream = _mongoc_stream_buffered_get_base_stream;
   stream->stream.check_closed = _mongoc_stream_buffered_check_closed;

   stream->base_stream = base_stream;

   _mongoc_buffer_init (&stream->buffer, NULL, buffer_size, NULL, NULL);

   mongoc_counter_streams_active_inc ();

   return (mongoc_stream_t *) stream;
}
