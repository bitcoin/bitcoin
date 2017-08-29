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

#ifndef MONGOC_ASYNC_PRIVATE_H
#define MONGOC_ASYNC_PRIVATE_H

#if !defined(MONGOC_COMPILATION)
#error "Only <mongoc.h> can be included directly."
#endif

#include <bson.h>
#include "mongoc-stream.h"

BSON_BEGIN_DECLS

struct _mongoc_async_cmd;

typedef struct _mongoc_async {
   struct _mongoc_async_cmd *cmds;
   size_t ncmds;
   uint32_t request_id;
} mongoc_async_t;

typedef enum {
   MONGOC_ASYNC_CMD_IN_PROGRESS,
   MONGOC_ASYNC_CMD_SUCCESS,
   MONGOC_ASYNC_CMD_ERROR,
   MONGOC_ASYNC_CMD_TIMEOUT,
} mongoc_async_cmd_result_t;

typedef void (*mongoc_async_cmd_cb_t) (mongoc_async_cmd_result_t result,
                                       const bson_t *bson,
                                       int64_t rtt_msec,
                                       void *data,
                                       bson_error_t *error);

typedef int (*mongoc_async_cmd_setup_t) (mongoc_stream_t *stream,
                                         int *events,
                                         void *ctx,
                                         int32_t timeout_msec,
                                         bson_error_t *error);


mongoc_async_t *
mongoc_async_new ();

void
mongoc_async_destroy (mongoc_async_t *async);

void
mongoc_async_run (mongoc_async_t *async);

BSON_END_DECLS

#endif /* MONGOC_ASYNC_PRIVATE_H */
