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

#ifndef MONGOC_CLIENT_PRIVATE_H
#define MONGOC_CLIENT_PRIVATE_H

#if !defined(MONGOC_COMPILATION)
#error "Only <mongoc.h> can be included directly."
#endif

#include <bson.h>

#include "mongoc-apm-private.h"
#include "mongoc-buffer-private.h"
#include "mongoc-client.h"
#include "mongoc-cluster-private.h"
#include "mongoc-config.h"
#include "mongoc-host-list.h"
#include "mongoc-read-prefs.h"
#include "mongoc-rpc-private.h"
#include "mongoc-opcode.h"
#ifdef MONGOC_ENABLE_SSL
#include "mongoc-ssl.h"
#endif
#include "mongoc-stream.h"
#include "mongoc-topology-private.h"
#include "mongoc-write-concern.h"


BSON_BEGIN_DECLS

/* protocol versions this driver can speak */
#define WIRE_VERSION_MIN 0
#define WIRE_VERSION_MAX 5

/* first version that supported aggregation cursors */
#define WIRE_VERSION_AGG_CURSOR 1
/* first version that supported "insert", "update", "delete" commands */
#define WIRE_VERSION_WRITE_CMD 2
/* first version when SCRAM-SHA-1 replaced MONGODB-CR as default auth mech */
#define WIRE_VERSION_SCRAM_DEFAULT 3
/* first version that supported "find" and "getMore" commands */
#define WIRE_VERSION_FIND_CMD 4
/* first version with "killCursors" command */
#define WIRE_VERSION_KILLCURSORS_CMD 4
/* first version when findAndModify accepts writeConcern */
#define WIRE_VERSION_FAM_WRITE_CONCERN 4
/* first version to support readConcern */
#define WIRE_VERSION_READ_CONCERN 4
/* first version to support maxStalenessSeconds */
#define WIRE_VERSION_MAX_STALENESS 5
/* first version to support writeConcern */
#define WIRE_VERSION_CMD_WRITE_CONCERN 5
/* first version to support collation */
#define WIRE_VERSION_COLLATION 5


struct _mongoc_client_t {
   mongoc_uri_t *uri;
   mongoc_cluster_t cluster;
   bool in_exhaust;

   mongoc_stream_initiator_t initiator;
   void *initiator_data;

#ifdef MONGOC_ENABLE_SSL
   bool use_ssl;
   mongoc_ssl_opt_t ssl_opts;
#endif

   mongoc_topology_t *topology;

   mongoc_read_prefs_t *read_prefs;
   mongoc_read_concern_t *read_concern;
   mongoc_write_concern_t *write_concern;

   mongoc_apm_callbacks_t apm_callbacks;
   void *apm_context;

   int32_t error_api_version;
   bool error_api_set;
};


/* Defines whether _mongoc_client_command_with_opts() is acting as a read
 * command helper for a command like "distinct", or a write command helper for
 * a command like "createRole", or both, like "aggregate" with "$out".
 */
typedef enum {
   MONGOC_CMD_READ = 1,
   MONGOC_CMD_WRITE = 2,
   MONGOC_CMD_RW = 3,
} mongoc_command_mode_t;

BSON_STATIC_ASSERT (MONGOC_CMD_RW == (MONGOC_CMD_READ | MONGOC_CMD_WRITE));


mongoc_client_t *
_mongoc_client_new_from_uri (const mongoc_uri_t *uri,
                             mongoc_topology_t *topology);

bool
_mongoc_client_set_apm_callbacks_private (mongoc_client_t *client,
                                          mongoc_apm_callbacks_t *callbacks,
                                          void *context);

mongoc_stream_t *
mongoc_client_default_stream_initiator (const mongoc_uri_t *uri,
                                        const mongoc_host_list_t *host,
                                        void *user_data,
                                        bson_error_t *error);

mongoc_stream_t *
_mongoc_client_create_stream (mongoc_client_t *client,
                              const mongoc_host_list_t *host,
                              bson_error_t *error);

bool
_mongoc_client_recv (mongoc_client_t *client,
                     mongoc_rpc_t *rpc,
                     mongoc_buffer_t *buffer,
                     mongoc_server_stream_t *server_stream,
                     bson_error_t *error);

bool
_mongoc_client_recv_gle (mongoc_client_t *client,
                         mongoc_server_stream_t *server_stream,
                         bson_t **gle_doc,
                         bson_error_t *error);

void
_mongoc_client_kill_cursor (mongoc_client_t *client,
                            uint32_t server_id,
                            int64_t cursor_id,
                            int64_t operation_id,
                            const char *db,
                            const char *collection);
bool
_mongoc_client_command_with_opts (mongoc_client_t *client,
                                  const char *db_name,
                                  const bson_t *command,
                                  mongoc_command_mode_t mode,
                                  const bson_t *opts,
                                  mongoc_query_flags_t flags,
                                  const mongoc_read_prefs_t *default_prefs,
                                  mongoc_read_concern_t *default_rc,
                                  mongoc_write_concern_t *default_wc,
                                  bson_t *reply,
                                  bson_error_t *error);

BSON_END_DECLS


#endif /* MONGOC_CLIENT_PRIVATE_H */
