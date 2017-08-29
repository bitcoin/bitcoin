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


#ifndef DEBUG_STREAM_H
#define DEBUG_STREAM_H

#include <mongoc.h>
#include <mongoc-client-private.h>

#include "test-libmongoc.h"


#define MONGOC_STREAM_DEBUG 7
typedef struct _mongoc_stream_debug_t {
   mongoc_stream_t vtable;
   mongoc_stream_t *wrapped;
   debug_stream_stats_t *stats;
} mongoc_stream_debug_t;


static int
_mongoc_stream_debug_close (mongoc_stream_t *stream)
{
   return mongoc_stream_close (((mongoc_stream_debug_t *) stream)->wrapped);
}


static void
_mongoc_stream_debug_destroy (mongoc_stream_t *stream)
{
   mongoc_stream_debug_t *debug_stream = (mongoc_stream_debug_t *) stream;

   debug_stream->stats->n_destroyed++;

   mongoc_stream_destroy (debug_stream->wrapped);
   bson_free (debug_stream);
}


static void
_mongoc_stream_debug_failed (mongoc_stream_t *stream)
{
   mongoc_stream_debug_t *debug_stream = (mongoc_stream_debug_t *) stream;

   debug_stream->stats->n_failed++;

   mongoc_stream_failed (debug_stream->wrapped);
   bson_free (debug_stream);
}


static int
_mongoc_stream_debug_setsockopt (mongoc_stream_t *stream,
                                 int level,
                                 int optname,
                                 void *optval,
                                 mongoc_socklen_t optlen)
{
   return mongoc_stream_setsockopt (((mongoc_stream_debug_t *) stream)->wrapped,
                                    level,
                                    optname,
                                    optval,
                                    optlen);
}


static int
_mongoc_stream_debug_flush (mongoc_stream_t *stream)
{
   return mongoc_stream_flush (((mongoc_stream_debug_t *) stream)->wrapped);
}


static ssize_t
_mongoc_stream_debug_readv (mongoc_stream_t *stream,
                            mongoc_iovec_t *iov,
                            size_t iovcnt,
                            size_t min_bytes,
                            int32_t timeout_msec)
{
   return mongoc_stream_readv (((mongoc_stream_debug_t *) stream)->wrapped,
                               iov,
                               iovcnt,
                               min_bytes,
                               timeout_msec);
}


static ssize_t
_mongoc_stream_debug_writev (mongoc_stream_t *stream,
                             mongoc_iovec_t *iov,
                             size_t iovcnt,
                             int32_t timeout_msec)
{
   return mongoc_stream_writev (
      ((mongoc_stream_debug_t *) stream)->wrapped, iov, iovcnt, timeout_msec);
}


static bool
_mongoc_stream_debug_check_closed (mongoc_stream_t *stream)
{
   return mongoc_stream_check_closed (
      ((mongoc_stream_debug_t *) stream)->wrapped);
}


static mongoc_stream_t *
_mongoc_stream_debug_get_base_stream (mongoc_stream_t *stream)
{
   mongoc_stream_t *wrapped = ((mongoc_stream_debug_t *) stream)->wrapped;

   /* "wrapped" is typically a mongoc_stream_buffered_t, get the real
    * base stream */
   if (wrapped->get_base_stream) {
      return wrapped->get_base_stream (wrapped);
   }

   return wrapped;
}


mongoc_stream_t *
debug_stream_new (mongoc_stream_t *stream, debug_stream_stats_t *stats)
{
   mongoc_stream_debug_t *debug_stream;

   if (!stream) {
      return NULL;
   }

   debug_stream = (mongoc_stream_debug_t *) bson_malloc0 (sizeof *debug_stream);

   debug_stream->vtable.type = MONGOC_STREAM_DEBUG;
   debug_stream->vtable.close = _mongoc_stream_debug_close;
   debug_stream->vtable.destroy = _mongoc_stream_debug_destroy;
   debug_stream->vtable.failed = _mongoc_stream_debug_failed;
   debug_stream->vtable.flush = _mongoc_stream_debug_flush;
   debug_stream->vtable.readv = _mongoc_stream_debug_readv;
   debug_stream->vtable.writev = _mongoc_stream_debug_writev;
   debug_stream->vtable.setsockopt = _mongoc_stream_debug_setsockopt;
   debug_stream->vtable.check_closed = _mongoc_stream_debug_check_closed;
   debug_stream->vtable.get_base_stream = _mongoc_stream_debug_get_base_stream;

   debug_stream->wrapped = stream;
   debug_stream->stats = stats;

   return (mongoc_stream_t *) debug_stream;
}


mongoc_stream_t *
debug_stream_initiator (const mongoc_uri_t *uri,
                        const mongoc_host_list_t *host,
                        void *user_data,
                        bson_error_t *error)
{
   debug_stream_stats_t *stats;
   mongoc_stream_t *default_stream;

   stats = (debug_stream_stats_t *) user_data;

   default_stream =
      mongoc_client_default_stream_initiator (uri, host, stats->client, error);

   return debug_stream_new (default_stream, stats);
}


void
test_framework_set_debug_stream (mongoc_client_t *client,
                                 debug_stream_stats_t *stats)
{
   stats->client = client;
   mongoc_client_set_stream_initiator (client, debug_stream_initiator, stats);
}

#endif /* DEBUG_STREAM_H */
