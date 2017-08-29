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

#ifndef MONGOC_BUFFER_PRIVATE_H
#define MONGOC_BUFFER_PRIVATE_H

#if !defined(MONGOC_COMPILATION)
#error "Only <mongoc.h> can be included directly."
#endif

#include <bson.h>

#include "mongoc-stream.h"


BSON_BEGIN_DECLS


typedef struct _mongoc_buffer_t mongoc_buffer_t;


struct _mongoc_buffer_t {
   uint8_t *data;
   size_t datalen;
   off_t off;
   size_t len;
   bson_realloc_func realloc_func;
   void *realloc_data;
};


void
_mongoc_buffer_init (mongoc_buffer_t *buffer,
                     uint8_t *buf,
                     size_t buflen,
                     bson_realloc_func realloc_func,
                     void *realloc_data);

bool
_mongoc_buffer_append_from_stream (mongoc_buffer_t *buffer,
                                   mongoc_stream_t *stream,
                                   size_t size,
                                   int32_t timeout_msec,
                                   bson_error_t *error);

ssize_t
_mongoc_buffer_try_append_from_stream (mongoc_buffer_t *buffer,
                                       mongoc_stream_t *stream,
                                       size_t size,
                                       int32_t timeout_msec);

ssize_t
_mongoc_buffer_fill (mongoc_buffer_t *buffer,
                     mongoc_stream_t *stream,
                     size_t min_bytes,
                     int32_t timeout_msec,
                     bson_error_t *error);

void
_mongoc_buffer_destroy (mongoc_buffer_t *buffer);

void
_mongoc_buffer_clear (mongoc_buffer_t *buffer, bool zero);


BSON_END_DECLS


#endif /* MONGOC_BUFFER_PRIVATE_H */
