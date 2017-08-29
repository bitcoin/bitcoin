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

#ifndef MONGOC_CURSOR_FILTER_PRIVATE_H
#define MONGOC_CURSOR_FILTER_PRIVATE_H

#if !defined(MONGOC_COMPILATION)
#error "Only <mongoc.h> can be included directly."
#endif

#include <bson.h>

#include "mongoc-cursor-private.h"


BSON_BEGIN_DECLS

typedef enum {
   MONGO_CURSOR_TRANSFORM_DROP,
   MONGO_CURSOR_TRANSFORM_PASS,
   MONGO_CURSOR_TRANSFORM_MUTATE,
} mongoc_cursor_transform_mode_t;

typedef mongoc_cursor_transform_mode_t (*mongoc_cursor_transform_filter_t) (
   const bson_t *bson, void *ctx);

typedef void (*mongoc_cursor_transform_mutate_t) (const bson_t *bson,
                                                  bson_t *out,
                                                  void *ctx);

typedef void (*mongoc_cursor_transform_dtor_t) (void *ctx);

void
_mongoc_cursor_transform_init (mongoc_cursor_t *cursor,
                               mongoc_cursor_transform_filter_t filter,
                               mongoc_cursor_transform_mutate_t mutate,
                               mongoc_cursor_transform_dtor_t dtor,
                               void *ctx);


BSON_END_DECLS


#endif /* MONGOC_CURSOR_FILTER_PRIVATE_H */
