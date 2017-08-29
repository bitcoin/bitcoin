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

#ifndef MONGOC_TOPOLOGY_PRIVATE_H
#define MONGOC_TOPOLOGY_PRIVATE_H

#include "mongoc-read-prefs-private.h"
#include "mongoc-topology-scanner-private.h"
#include "mongoc-server-description-private.h"
#include "mongoc-topology-description-private.h"
#include "mongoc-thread-private.h"
#include "mongoc-uri.h"

#define MONGOC_TOPOLOGY_MIN_HEARTBEAT_FREQUENCY_MS 500
#define MONGOC_TOPOLOGY_SOCKET_CHECK_INTERVAL_MS 5000
#define MONGOC_TOPOLOGY_COOLDOWN_MS 5000
#define MONGOC_TOPOLOGY_LOCAL_THRESHOLD_MS 15
#define MONGOC_TOPOLOGY_SERVER_SELECTION_TIMEOUT_MS 30000
#define MONGOC_TOPOLOGY_HEARTBEAT_FREQUENCY_MS_MULTI_THREADED 10000
#define MONGOC_TOPOLOGY_HEARTBEAT_FREQUENCY_MS_SINGLE_THREADED 60000

typedef enum {
   MONGOC_TOPOLOGY_SCANNER_OFF,
   MONGOC_TOPOLOGY_SCANNER_BG_RUNNING,
   MONGOC_TOPOLOGY_SCANNER_SHUTTING_DOWN,
   MONGOC_TOPOLOGY_SCANNER_SINGLE_THREADED,
} mongoc_topology_scanner_state_t;

typedef struct _mongoc_topology_t {
   mongoc_topology_description_t description;
   mongoc_uri_t *uri;
   mongoc_topology_scanner_t *scanner;
   bool server_selection_try_once;

   int64_t last_scan;
   int64_t local_threshold_msec;
   int64_t connect_timeout_msec;
   int64_t server_selection_timeout_msec;

   mongoc_mutex_t mutex;
   mongoc_cond_t cond_client;
   mongoc_cond_t cond_server;
   mongoc_thread_t thread;

   mongoc_topology_scanner_state_t scanner_state;
   bool scan_requested;
   bool shutdown_requested;
   bool single_threaded;
   bool stale;
} mongoc_topology_t;

mongoc_topology_t *
mongoc_topology_new (const mongoc_uri_t *uri, bool single_threaded);

void
mongoc_topology_set_apm_callbacks (mongoc_topology_t *topology,
                                   mongoc_apm_callbacks_t *callbacks,
                                   void *context);

void
mongoc_topology_destroy (mongoc_topology_t *topology);

bool
mongoc_topology_compatible (const mongoc_topology_description_t *td,
                            const mongoc_read_prefs_t *read_prefs,
                            bson_error_t *error);

mongoc_server_description_t *
mongoc_topology_select (mongoc_topology_t *topology,
                        mongoc_ss_optype_t optype,
                        const mongoc_read_prefs_t *read_prefs,
                        bson_error_t *error);

uint32_t
mongoc_topology_select_server_id (mongoc_topology_t *topology,
                                  mongoc_ss_optype_t optype,
                                  const mongoc_read_prefs_t *read_prefs,
                                  bson_error_t *error);

mongoc_server_description_t *
mongoc_topology_server_by_id (mongoc_topology_t *topology,
                              uint32_t id,
                              bson_error_t *error);

mongoc_host_list_t *
_mongoc_topology_host_by_id (mongoc_topology_t *topology,
                             uint32_t id,
                             bson_error_t *error);

void
mongoc_topology_invalidate_server (mongoc_topology_t *topology,
                                   uint32_t id,
                                   const bson_error_t *error);

bool
_mongoc_topology_update_from_handshake (mongoc_topology_t *topology,
                                        const mongoc_server_description_t *sd);

int64_t
mongoc_topology_server_timestamp (mongoc_topology_t *topology, uint32_t id);

mongoc_topology_description_type_t
_mongoc_topology_get_type (mongoc_topology_t *topology);

bool
_mongoc_topology_start_background_scanner (mongoc_topology_t *topology);

bool
_mongoc_topology_set_appname (mongoc_topology_t *topology, const char *appname);
#endif
