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

#ifndef MONGOC_SERVER_DESCRIPTION_PRIVATE_H
#define MONGOC_SERVER_DESCRIPTION_PRIVATE_H

#include "mongoc-server-description.h"


#define MONGOC_DEFAULT_WIRE_VERSION 0
#define MONGOC_DEFAULT_WRITE_BATCH_SIZE 1000
#define MONGOC_DEFAULT_BSON_OBJ_SIZE 16 * 1024 * 1024
#define MONGOC_DEFAULT_MAX_MSG_SIZE 48000000
#define MONGOC_IDLE_WRITE_PERIOD_MS 10 * 1000

/* represent a server or topology with no replica set config version */
#define MONGOC_NO_SET_VERSION -1

typedef enum {
   MONGOC_SERVER_UNKNOWN,
   MONGOC_SERVER_STANDALONE,
   MONGOC_SERVER_MONGOS,
   MONGOC_SERVER_POSSIBLE_PRIMARY,
   MONGOC_SERVER_RS_PRIMARY,
   MONGOC_SERVER_RS_SECONDARY,
   MONGOC_SERVER_RS_ARBITER,
   MONGOC_SERVER_RS_OTHER,
   MONGOC_SERVER_RS_GHOST,
   MONGOC_SERVER_DESCRIPTION_TYPES,
} mongoc_server_description_type_t;

struct _mongoc_server_description_t {
   uint32_t id;
   mongoc_host_list_t host;
   int64_t round_trip_time_msec;
   int64_t last_update_time_usec;
   bson_t last_is_master;
   bool has_is_master;
   const char *connection_address;
   const char *me;

   /* whether an APM server-opened callback has been fired before */
   bool opened;

   /* The following fields are filled from the last_is_master and are zeroed on
    * parse.  So order matters here.  DON'T move set_name */
   const char *set_name;
   bson_error_t error;
   mongoc_server_description_type_t type;

   int32_t min_wire_version;
   int32_t max_wire_version;
   int32_t max_msg_size;
   int32_t max_bson_obj_size;
   int32_t max_write_batch_size;

   bson_t hosts;
   bson_t passives;
   bson_t arbiters;

   bson_t tags;
   const char *current_primary;
   int64_t set_version;
   bson_oid_t election_id;
   int64_t last_write_date_ms;

#ifdef MONGOC_ENABLE_COMPRESSION
   bson_t compressors;
#endif
};

void
mongoc_server_description_init (mongoc_server_description_t *sd,
                                const char *address,
                                uint32_t id);
bool
mongoc_server_description_has_rs_member (
   mongoc_server_description_t *description, const char *address);


bool
mongoc_server_description_has_set_version (
   mongoc_server_description_t *description);

bool
mongoc_server_description_has_election_id (
   mongoc_server_description_t *description);

void
mongoc_server_description_cleanup (mongoc_server_description_t *sd);

void
mongoc_server_description_reset (mongoc_server_description_t *sd);

void
mongoc_server_description_set_state (mongoc_server_description_t *description,
                                     mongoc_server_description_type_t type);
void
mongoc_server_description_set_set_version (
   mongoc_server_description_t *description, int64_t set_version);
void
mongoc_server_description_set_election_id (
   mongoc_server_description_t *description, const bson_oid_t *election_id);
void
mongoc_server_description_update_rtt (mongoc_server_description_t *server,
                                      int64_t rtt_msec);

void
mongoc_server_description_handle_ismaster (mongoc_server_description_t *sd,
                                           const bson_t *reply,
                                           int64_t rtt_msec,
                                           const bson_error_t *error /* IN */);

void
mongoc_server_description_filter_stale (mongoc_server_description_t **sds,
                                        size_t sds_len,
                                        mongoc_server_description_t *primary,
                                        int64_t heartbeat_frequency_ms,
                                        const mongoc_read_prefs_t *read_prefs);

void
mongoc_server_description_filter_tags (
   mongoc_server_description_t **descriptions,
   size_t description_len,
   const mongoc_read_prefs_t *read_prefs);

#endif
