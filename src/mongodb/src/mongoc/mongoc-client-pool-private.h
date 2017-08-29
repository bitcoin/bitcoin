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

#ifndef MONGOC_CLIENT_POOL_PRIVATE_H
#define MONGOC_CLIENT_POOL_PRIVATE_H

#if !defined(MONGOC_COMPILATION)
#error "Only <mongoc.h> can be included directly."
#endif

#include <bson.h>

#include "mongoc-client-pool.h"
#include "mongoc-topology-description.h"
#include "mongoc-topology-private.h"

BSON_BEGIN_DECLS

/* for tests */
void
_mongoc_client_pool_set_stream_initiator (mongoc_client_pool_t *pool,
                                          mongoc_stream_initiator_t si,
                                          void *user_data);
size_t
mongoc_client_pool_get_size (mongoc_client_pool_t *pool);
size_t
mongoc_client_pool_num_pushed (mongoc_client_pool_t *pool);
mongoc_topology_t *
_mongoc_client_pool_get_topology (mongoc_client_pool_t *pool);

BSON_END_DECLS


#endif /* MONGOC_CLIENT_POOL_PRIVATE_H */
