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

#ifndef MONGOC_ASYNC_CMD_PRIVATE_H
#define MONGOC_ASYNC_CMD_PRIVATE_H

#if !defined(MONGOC_COMPILATION)
#error "Only <mongoc.h> can be included directly."
#endif

#include <bson.h>

#include "mongoc-client.h"
#include "mongoc-async-private.h"
#include "mongoc-array-private.h"
#include "mongoc-buffer-private.h"
#include "mongoc-rpc-private.h"
#include "mongoc-stream.h"

BSON_BEGIN_DECLS

typedef enum {
   MONGOC_ASYNC_CMD_SETUP,
   MONGOC_ASYNC_CMD_SEND,
   MONGOC_ASYNC_CMD_RECV_LEN,
   MONGOC_ASYNC_CMD_RECV_RPC,
   MONGOC_ASYNC_CMD_ERROR_STATE,
   MONGOC_ASYNC_CMD_CANCELED_STATE,
} mongoc_async_cmd_state_t;

typedef struct _mongoc_async_cmd {
   mongoc_stream_t *stream;

   mongoc_async_t *async;
   mongoc_async_cmd_state_t state;
   int events;
   mongoc_async_cmd_setup_t setup;
   void *setup_ctx;
   mongoc_async_cmd_cb_t cb;
   void *data;
   bson_error_t error;
   int64_t connect_started;
   int64_t cmd_started;
   int64_t timeout_msec;
   bson_t cmd;
   mongoc_buffer_t buffer;
   mongoc_array_t array;
   mongoc_iovec_t *iovec;
   size_t niovec;
   size_t bytes_to_read;
   mongoc_rpc_t rpc;
   bson_t reply;
   bool reply_needs_cleanup;
   char ns[MONGOC_NAMESPACE_MAX];

   struct _mongoc_async_cmd *next;
   struct _mongoc_async_cmd *prev;
} mongoc_async_cmd_t;

mongoc_async_cmd_t *
mongoc_async_cmd_new (mongoc_async_t *async,
                      mongoc_stream_t *stream,
                      mongoc_async_cmd_setup_t setup,
                      void *setup_ctx,
                      const char *dbname,
                      const bson_t *cmd,
                      mongoc_async_cmd_cb_t cb,
                      void *cb_data,
                      int64_t timeout_msec);

void
mongoc_async_cmd_destroy (mongoc_async_cmd_t *acmd);

bool
mongoc_async_cmd_run (mongoc_async_cmd_t *acmd);

#ifdef MONGOC_ENABLE_SSL
int
mongoc_async_cmd_tls_setup (mongoc_stream_t *stream,
                            int *events,
                            void *ctx,
                            int32_t timeout_msec,
                            bson_error_t *error);
#endif

BSON_END_DECLS


#endif /* MONGOC_ASYNC_CMD_PRIVATE_H */
