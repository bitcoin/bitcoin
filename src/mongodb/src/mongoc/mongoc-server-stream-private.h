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

#ifndef MONGOC_SERVER_STREAM_H
#define MONGOC_SERVER_STREAM_H

#include "mongoc-config.h"

#if !defined(MONGOC_COMPILATION)
#error "Only <mongoc.h> can be included directly."
#endif

#include <bson.h>

#include "mongoc-topology-description-private.h"
#include "mongoc-server-description-private.h"
#include "mongoc-stream.h"

BSON_BEGIN_DECLS

typedef struct _mongoc_server_stream_t {
   mongoc_topology_description_type_t topology_type;
   mongoc_server_description_t *sd; /* owned */
   mongoc_stream_t *stream;         /* borrowed */
} mongoc_server_stream_t;


mongoc_server_stream_t *
mongoc_server_stream_new (mongoc_topology_description_type_t topology_type,
                          mongoc_server_description_t *sd,
                          mongoc_stream_t *stream);

int32_t
mongoc_server_stream_max_bson_obj_size (mongoc_server_stream_t *server_stream);

int32_t
mongoc_server_stream_max_msg_size (mongoc_server_stream_t *server_stream);

int32_t
mongoc_server_stream_max_write_batch_size (
   mongoc_server_stream_t *server_stream);

void
mongoc_server_stream_cleanup (mongoc_server_stream_t *server_stream);

BSON_END_DECLS


#endif /* MONGOC_SERVER_STREAM_H */
