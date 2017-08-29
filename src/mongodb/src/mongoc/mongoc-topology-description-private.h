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

#ifndef MONGOC_TOPOLOGY_DESCRIPTION_PRIVATE_H
#define MONGOC_TOPOLOGY_DESCRIPTION_PRIVATE_H

#include "mongoc-set-private.h"
#include "mongoc-server-description.h"
#include "mongoc-array-private.h"
#include "mongoc-topology-description.h"
#include "mongoc-apm-private.h"


typedef enum {
   MONGOC_TOPOLOGY_UNKNOWN,
   MONGOC_TOPOLOGY_SHARDED,
   MONGOC_TOPOLOGY_RS_NO_PRIMARY,
   MONGOC_TOPOLOGY_RS_WITH_PRIMARY,
   MONGOC_TOPOLOGY_SINGLE,
   MONGOC_TOPOLOGY_DESCRIPTION_TYPES
} mongoc_topology_description_type_t;

struct _mongoc_topology_description_t {
   bson_oid_t topology_id;
   bool opened;
   mongoc_topology_description_type_t type;
   int64_t heartbeat_msec;
   mongoc_set_t *servers;
   char *set_name;
   int64_t max_set_version;
   bson_oid_t max_election_id;
   bson_error_t compatibility_error;
   uint32_t max_server_id;
   bool stale;
   unsigned int rand_seed;

   mongoc_apm_callbacks_t apm_callbacks;
   void *apm_context;
};

typedef enum { MONGOC_SS_READ, MONGOC_SS_WRITE } mongoc_ss_optype_t;

void
mongoc_topology_description_init (mongoc_topology_description_t *description,
                                  mongoc_topology_description_type_t type,
                                  int64_t heartbeat_msec);

void
_mongoc_topology_description_copy_to (const mongoc_topology_description_t *src,
                                      mongoc_topology_description_t *dst);

void
mongoc_topology_description_destroy (
   mongoc_topology_description_t *description);

void
mongoc_topology_description_handle_ismaster (
   mongoc_topology_description_t *topology,
   uint32_t server_id,
   const bson_t *reply,
   int64_t rtt_msec,
   const bson_error_t *error /* IN */);

mongoc_server_description_t *
mongoc_topology_description_select (mongoc_topology_description_t *description,
                                    mongoc_ss_optype_t optype,
                                    const mongoc_read_prefs_t *read_pref,
                                    int64_t local_threshold_ms);

mongoc_server_description_t *
mongoc_topology_description_server_by_id (
   mongoc_topology_description_t *description,
   uint32_t id,
   bson_error_t *error);

int32_t
mongoc_topology_description_lowest_max_wire_version (
   const mongoc_topology_description_t *td);

bool
mongoc_topology_description_all_sds_have_write_date (
   const mongoc_topology_description_t *td);

bool
_mongoc_topology_description_validate_max_staleness (
   const mongoc_topology_description_t *td,
   int64_t max_staleness_seconds,
   bson_error_t *error);

void
mongoc_topology_description_suitable_servers (
   mongoc_array_t *set, /* OUT */
   mongoc_ss_optype_t optype,
   mongoc_topology_description_t *topology,
   const mongoc_read_prefs_t *read_pref,
   size_t local_threshold_ms);

void
mongoc_topology_description_invalidate_server (
   mongoc_topology_description_t *topology,
   uint32_t id,
   const bson_error_t *error /* IN */);

bool
mongoc_topology_description_add_server (mongoc_topology_description_t *topology,
                                        const char *server,
                                        uint32_t *id /* OUT */);

#endif /* MONGOC_TOPOLOGY_DESCRIPTION_PRIVATE_H */
