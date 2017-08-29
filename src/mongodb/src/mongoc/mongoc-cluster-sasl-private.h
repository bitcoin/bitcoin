/*
 * Copyright 2017 MongoDB, Inc.
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

#ifndef MONGOC_CLUSTER_SASL_PRIVATE_H
#define MONGOC_CLUSTER_SASL_PRIVATE_H

#if !defined(MONGOC_COMPILATION)
#error "Only <mongoc.h> can be included directly."
#endif

#include "mongoc-config.h"
#include "mongoc-cluster-private.h"
#include <bson.h>

bool
_mongoc_cluster_auth_node_sasl (mongoc_cluster_t *cluster,
                                mongoc_stream_t *stream,
                                const char *hostname,
                                bson_error_t *error);
#endif /* MONGOC_CLUSTER_SASL_PRIVATE_H */
