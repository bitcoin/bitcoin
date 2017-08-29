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
#include <stdarg.h>

#include "mongoc-error.h"
#include "mongoc-buffer-private.h"
#include "mongoc-trace-private.h"


#undef MONGOC_LOG_DOMAIN
#define MONGOC_LOG_DOMAIN "buffer"

#ifndef MONGOC_BUFFER_DEFAULT_SIZE
#define MONGOC_BUFFER_DEFAULT_SIZE 1024
#endif


#define SPACE_FOR(_b, _sz)                                                   \
   (((ssize_t) (_b)->datalen - (ssize_t) (_b)->off - (ssize_t) (_b)->len) >= \
    (ssize_t) (_sz))


/**
 * _mongoc_buffer_init:
 * @buffer: A mongoc_buffer_t to initialize.
 * @buf: A data buffer to attach to @buffer.
 * @buflen: The size of @buflen.
 * @realloc_func: A function to resize @buf.
 *
 * Initializes @buffer for use. If additional space is needed by @buffer, then
 * @realloc_func will be called to resize @buf.
 *
 * @buffer takes ownership of @buf and will realloc it to zero bytes when
 * cleaning up the data structure.
 */
void
_mongoc_buffer_init (mongoc_buffer_t *buffer,
                     uint8_t *buf,
                     size_t buflen,
                     bson_realloc_func realloc_func,
                     void *realloc_data)
{
   BSON_ASSERT (buffer);
   BSON_ASSERT (buflen || !buf);

   if (!realloc_func) {
      realloc_func = bson_realloc_ctx;
   }

   if (!buflen) {
      buflen = MONGOC_BUFFER_DEFAULT_SIZE;
   }

   if (!buf) {
      buf = (uint8_t *) realloc_func (NULL, buflen, NULL);
   }

   memset (buffer, 0, sizeof *buffer);

   buffer->data = buf;
   buffer->datalen = buflen;
   buffer->len = 0;
   buffer->off = 0;
   buffer->realloc_func = realloc_func;
   buffer->realloc_data = realloc_data;
}


/**
 * _mongoc_buffer_destroy:
 * @buffer: A mongoc_buffer_t.
 *
 * Cleanup after @buffer and release any allocated resources.
 */
void
_mongoc_buffer_destroy (mongoc_buffer_t *buffer)
{
   BSON_ASSERT (buffer);

   if (buffer->data && buffer->realloc_func) {
      buffer->realloc_func (buffer->data, 0, buffer->realloc_data);
   }

   memset (buffer, 0, sizeof *buffer);
}


/**
 * _mongoc_buffer_clear:
 * @buffer: A mongoc_buffer_t.
 * @zero: If the memory should be zeroed.
 *
 * Clears a buffers contents and resets it to initial state. You can request
 * that the memory is zeroed, which might be useful if you know the contents
 * contain security related information.
 */
void
_mongoc_buffer_clear (mongoc_buffer_t *buffer, bool zero)
{
   BSON_ASSERT (buffer);

   if (zero) {
      memset (buffer->data, 0, buffer->datalen);
   }

   buffer->off = 0;
   buffer->len = 0;
}


/**
 * mongoc_buffer_append_from_stream:
 * @buffer; A mongoc_buffer_t.
 * @stream: The stream to read from.
 * @size: The number of bytes to read.
 * @timeout_msec: The number of milliseconds to wait or -1 for the default
 * @error: A location for a bson_error_t, or NULL.
 *
 * Reads from stream @size bytes and stores them in @buffer. This can be used
 * in conjunction with reading RPCs from a stream. You read from the stream
 * into this buffer and then scatter the buffer into the RPC.
 *
 * Returns: true if successful; otherwise false and @error is set.
 */
bool
_mongoc_buffer_append_from_stream (mongoc_buffer_t *buffer,
                                   mongoc_stream_t *stream,
                                   size_t size,
                                   int32_t timeout_msec,
                                   bson_error_t *error)
{
   uint8_t *buf;
   ssize_t ret;

   ENTRY;

   BSON_ASSERT (buffer);
   BSON_ASSERT (stream);
   BSON_ASSERT (size);

   BSON_ASSERT (buffer->datalen);
   BSON_ASSERT ((buffer->datalen + size) < INT_MAX);

   if (!SPACE_FOR (buffer, size)) {
      if (buffer->len) {
         memmove (&buffer->data[0], &buffer->data[buffer->off], buffer->len);
      }
      buffer->off = 0;
      if (!SPACE_FOR (buffer, size)) {
         buffer->datalen =
            bson_next_power_of_two (size + buffer->len + buffer->off);
         buffer->data = (uint8_t *) buffer->realloc_func (
            buffer->data, buffer->datalen, NULL);
      }
   }

   buf = &buffer->data[buffer->off + buffer->len];

   BSON_ASSERT ((buffer->off + buffer->len + size) <= buffer->datalen);

   ret = mongoc_stream_read (stream, buf, size, size, timeout_msec);
   if (ret != size) {
      bson_set_error (error,
                      MONGOC_ERROR_STREAM,
                      MONGOC_ERROR_STREAM_SOCKET,
                      "Failed to read %" PRIu64
                      " bytes: socket error or timeout",
                      (uint64_t) size);
      RETURN (false);
   }

   buffer->len += ret;

   RETURN (true);
}


/**
 * _mongoc_buffer_fill:
 * @buffer: A mongoc_buffer_t.
 * @stream: A stream to read from.
 * @min_bytes: The minumum number of bytes to read.
 * @error: A location for a bson_error_t or NULL.
 *
 * Attempts to fill the entire buffer, or at least @min_bytes.
 *
 * Returns: The number of buffered bytes, or -1 on failure.
 */
ssize_t
_mongoc_buffer_fill (mongoc_buffer_t *buffer,
                     mongoc_stream_t *stream,
                     size_t min_bytes,
                     int32_t timeout_msec,
                     bson_error_t *error)
{
   ssize_t ret;
   size_t avail_bytes;

   ENTRY;

   BSON_ASSERT (buffer);
   BSON_ASSERT (stream);

   BSON_ASSERT (buffer->data);
   BSON_ASSERT (buffer->datalen);

   if (min_bytes <= buffer->len) {
      RETURN (buffer->len);
   }

   min_bytes -= buffer->len;

   if (buffer->len) {
      memmove (&buffer->data[0], &buffer->data[buffer->off], buffer->len);
   }

   buffer->off = 0;

   if (!SPACE_FOR (buffer, min_bytes)) {
      buffer->datalen = bson_next_power_of_two (buffer->len + min_bytes);
      buffer->data = (uint8_t *) buffer->realloc_func (
         buffer->data, buffer->datalen, buffer->realloc_data);
   }

   avail_bytes = buffer->datalen - buffer->len;

   ret = mongoc_stream_read (stream,
                             &buffer->data[buffer->off + buffer->len],
                             avail_bytes,
                             min_bytes,
                             timeout_msec);

   if (ret == -1) {
      bson_set_error (error,
                      MONGOC_ERROR_STREAM,
                      MONGOC_ERROR_STREAM_SOCKET,
                      "Failed to buffer %u bytes",
                      (unsigned) min_bytes);
      RETURN (-1);
   }

   buffer->len += ret;

   if (buffer->len < min_bytes) {
      bson_set_error (error,
                      MONGOC_ERROR_STREAM,
                      MONGOC_ERROR_STREAM_SOCKET,
                      "Could only buffer %u of %u bytes",
                      (unsigned) buffer->len,
                      (unsigned) min_bytes);
      RETURN (-1);
   }

   RETURN (buffer->len);
}


/**
 * mongoc_buffer_try_append_from_stream:
 * @buffer; A mongoc_buffer_t.
 * @stream: The stream to read from.
 * @size: The number of bytes to read.
 * @timeout_msec: The number of milliseconds to wait or -1 for the default
 *
 * Reads from stream @size bytes and stores them in @buffer. This can be used
 * in conjunction with reading RPCs from a stream. You read from the stream
 * into this buffer and then scatter the buffer into the RPC.
 *
 * Returns: bytes read if successful; otherwise 0 or -1.
 */
ssize_t
_mongoc_buffer_try_append_from_stream (mongoc_buffer_t *buffer,
                                       mongoc_stream_t *stream,
                                       size_t size,
                                       int32_t timeout_msec)
{
   uint8_t *buf;
   ssize_t ret;

   ENTRY;

   BSON_ASSERT (buffer);
   BSON_ASSERT (stream);
   BSON_ASSERT (size);

   BSON_ASSERT (buffer->datalen);
   BSON_ASSERT ((buffer->datalen + size) < INT_MAX);

   if (!SPACE_FOR (buffer, size)) {
      if (buffer->len) {
         memmove (&buffer->data[0], &buffer->data[buffer->off], buffer->len);
      }
      buffer->off = 0;
      if (!SPACE_FOR (buffer, size)) {
         buffer->datalen =
            bson_next_power_of_two (size + buffer->len + buffer->off);
         buffer->data = (uint8_t *) buffer->realloc_func (
            buffer->data, buffer->datalen, NULL);
      }
   }

   buf = &buffer->data[buffer->off + buffer->len];

   BSON_ASSERT ((buffer->off + buffer->len + size) <= buffer->datalen);

   ret = mongoc_stream_read (stream, buf, size, 0, timeout_msec);

   if (ret > 0) {
      buffer->len += ret;
   }

   RETURN (ret);
}
