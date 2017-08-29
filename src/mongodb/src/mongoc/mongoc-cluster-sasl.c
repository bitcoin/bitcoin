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

/* for size_t */
#include <bson.h>
#include "mongoc-config.h"

#ifdef MONGOC_ENABLE_SASL
#include "mongoc-cluster-private.h"
#include "mongoc-log.h"
#include "mongoc-trace-private.h"
#include "mongoc-stream-private.h"
#include "mongoc-stream-socket.h"
#include "mongoc-error.h"
#include "mongoc-util-private.h"

#ifdef MONGOC_ENABLE_SASL_CYRUS
#include "mongoc-cluster-cyrus-private.h"
#endif
#ifdef MONGOC_ENABLE_SASL_SSPI
#include "mongoc-cluster-sspi-private.h"
#endif
#ifdef MONGOC_ENABLE_SASL_GSSAPI
#include "mongoc-cluster-gssapi-private.h"
#endif

void
_mongoc_cluster_build_sasl_start (bson_t *cmd,
                                  const char *mechanism,
                                  const char *buf,
                                  uint32_t buflen)
{
   BSON_APPEND_INT32 (cmd, "saslStart", 1);
   BSON_APPEND_UTF8 (cmd, "mechanism", "GSSAPI");
   bson_append_utf8 (cmd, "payload", 7, buf, buflen);
   BSON_APPEND_INT32 (cmd, "autoAuthorize", 1);
}
void
_mongoc_cluster_build_sasl_continue (bson_t *cmd,
                                     int conv_id,
                                     const char *buf,
                                     uint32_t buflen)
{
   BSON_APPEND_INT32 (cmd, "saslContinue", 1);
   BSON_APPEND_INT32 (cmd, "conversationId", conv_id);
   bson_append_utf8 (cmd, "payload", 7, buf, buflen);
}
int
_mongoc_cluster_get_conversation_id (const bson_t *reply)
{
   bson_iter_t iter;

   if (bson_iter_init_find (&iter, reply, "conversationId") &&
       BSON_ITER_HOLDS_INT32 (&iter)) {
      return bson_iter_int32 (&iter);
   }

   return 0;
}

/*
 *--------------------------------------------------------------------------
 *
 * _mongoc_cluster_auth_node_sasl --
 *
 *       Perform authentication for a cluster node using SASL. This is
 *       only supported for GSSAPI at the moment.
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
_mongoc_cluster_auth_node_sasl (mongoc_cluster_t *cluster,
                                mongoc_stream_t *stream,
                                const char *hostname,
                                bson_error_t *error)
{
#ifdef MONGOC_ENABLE_SASL_CYRUS
   return _mongoc_cluster_auth_node_cyrus (cluster, stream, hostname, error);
#endif
#ifdef MONGOC_ENABLE_SASL_SSPI
   return _mongoc_cluster_auth_node_sspi (cluster, stream, hostname, error);
#endif
#ifdef MONGOC_ENABLE_SASL_GSSAPI
   return _mongoc_cluster_auth_node_gssapi (cluster, stream, hostname, error);
#endif
}
#endif
