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

#include "mongoc-config.h"

#ifdef MONGOC_ENABLE_SASL_GSSAPI

#include "mongoc-cluster-gssapi-private.h"
#include "mongoc-cluster-sasl-private.h"
#include "mongoc-gssapi-private.h"
#include "mongoc-error.h"
#include "mongoc-util-private.h"


/*
 *--------------------------------------------------------------------------
 *
 * _mongoc_cluster_auth_node_gssapi --
 *
 *       Perform authentication for a cluster node using GSSAPI
 *
 * Returns:
 *       true if successful; otherwise false and @error is set.
 *
 * Side effects:
 *       error may be set.
 *
 *--------------------------------------------------------------------------
 */

bool
_mongoc_cluster_auth_node_gssapi (mongoc_cluster_t *cluster,
                                  mongoc_stream_t *stream,
                                  const char *hostname,
                                  bson_error_t *error)
{
   return false;
}

#endif
