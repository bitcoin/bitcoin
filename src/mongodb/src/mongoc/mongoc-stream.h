/*
 * Copyright 2013-2014 MongoDB, Inc.
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

#ifndef MONGOC_STREAM_H
#define MONGOC_STREAM_H

#if !defined(MONGOC_INSIDE) && !defined(MONGOC_COMPILATION)
#error "Only <mongoc.h> can be included directly."
#endif

#include "mongoc-macros.h"
#include "mongoc-iovec.h"
#include "mongoc-socket.h"


BSON_BEGIN_DECLS


typedef struct _mongoc_stream_t mongoc_stream_t;

typedef struct _mongoc_stream_poll_t {
   mongoc_stream_t *stream;
   int events;
   int revents;
} mongoc_stream_poll_t;

struct _mongoc_stream_t {
   int type;
   void (*destroy) (mongoc_stream_t *stream);
   int (*close) (mongoc_stream_t *stream);
   int (*flush) (mongoc_stream_t *stream);
   ssize_t (*writev) (mongoc_stream_t *stream,
                      mongoc_iovec_t *iov,
                      size_t iovcnt,
                      int32_t timeout_msec);
   ssize_t (*readv) (mongoc_stream_t *stream,
                     mongoc_iovec_t *iov,
                     size_t iovcnt,
                     size_t min_bytes,
                     int32_t timeout_msec);
   int (*setsockopt) (mongoc_stream_t *stream,
                      int level,
                      int optname,
                      void *optval,
                      mongoc_socklen_t optlen);
   mongoc_stream_t *(*get_base_stream) (mongoc_stream_t *stream);
   bool (*check_closed) (mongoc_stream_t *stream);
   ssize_t (*poll) (mongoc_stream_poll_t *streams,
                    size_t nstreams,
                    int32_t timeout);
   void (*failed) (mongoc_stream_t *stream);
   void *padding[5];
};


MONGOC_EXPORT (mongoc_stream_t *)
mongoc_stream_get_base_stream (mongoc_stream_t *stream);
MONGOC_EXPORT (mongoc_stream_t *)
mongoc_stream_get_tls_stream (mongoc_stream_t *stream);
MONGOC_EXPORT (int)
mongoc_stream_close (mongoc_stream_t *stream);
MONGOC_EXPORT (void)
mongoc_stream_destroy (mongoc_stream_t *stream);
MONGOC_EXPORT (void)
mongoc_stream_failed (mongoc_stream_t *stream);
MONGOC_EXPORT (int)
mongoc_stream_flush (mongoc_stream_t *stream);
MONGOC_EXPORT (ssize_t)
mongoc_stream_writev (mongoc_stream_t *stream,
                      mongoc_iovec_t *iov,
                      size_t iovcnt,
                      int32_t timeout_msec);
MONGOC_EXPORT (ssize_t)
mongoc_stream_write (mongoc_stream_t *stream,
                     void *buf,
                     size_t count,
                     int32_t timeout_msec);
MONGOC_EXPORT (ssize_t)
mongoc_stream_readv (mongoc_stream_t *stream,
                     mongoc_iovec_t *iov,
                     size_t iovcnt,
                     size_t min_bytes,
                     int32_t timeout_msec);
MONGOC_EXPORT (ssize_t)
mongoc_stream_read (mongoc_stream_t *stream,
                    void *buf,
                    size_t count,
                    size_t min_bytes,
                    int32_t timeout_msec);
MONGOC_EXPORT (int)
mongoc_stream_setsockopt (mongoc_stream_t *stream,
                          int level,
                          int optname,
                          void *optval,
                          mongoc_socklen_t optlen);
MONGOC_EXPORT (bool)
mongoc_stream_check_closed (mongoc_stream_t *stream);
MONGOC_EXPORT (ssize_t)
mongoc_stream_poll (mongoc_stream_poll_t *streams,
                    size_t nstreams,
                    int32_t timeout);


BSON_END_DECLS


#endif /* MONGOC_STREAM_H */
