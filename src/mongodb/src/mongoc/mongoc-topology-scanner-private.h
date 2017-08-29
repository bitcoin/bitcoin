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

#ifndef MONGOC_TOPOLOGY_SCANNER_PRIVATE_H
#define MONGOC_TOPOLOGY_SCANNER_PRIVATE_H

/* TODO: rename to TOPOLOGY scanner */

#if !defined(MONGOC_COMPILATION)
#error "Only <mongoc.h> can be included directly."
#endif

#include <bson.h>
#include "mongoc-async-private.h"
#include "mongoc-async-cmd-private.h"
#include "mongoc-host-list.h"
#include "mongoc-apm-private.h"

#ifdef MONGOC_ENABLE_SSL
#include "mongoc-ssl.h"
#endif

BSON_BEGIN_DECLS

typedef void (*mongoc_topology_scanner_setup_err_cb_t) (
   uint32_t id, void *data, const bson_error_t *error /* IN */);

typedef void (*mongoc_topology_scanner_cb_t) (
   uint32_t id,
   const bson_t *bson,
   int64_t rtt,
   void *data,
   const bson_error_t *error /* IN */);

struct mongoc_topology_scanner;

typedef struct mongoc_topology_scanner_node {
   uint32_t id;
   mongoc_async_cmd_t *cmd;
   mongoc_stream_t *stream;
   int64_t timestamp;
   int64_t last_used;
   int64_t last_failed;
   bool has_auth;
   mongoc_host_list_t host;
   struct addrinfo *dns_results;
   struct addrinfo *current_dns_result;
   struct mongoc_topology_scanner *ts;

   struct mongoc_topology_scanner_node *next;
   struct mongoc_topology_scanner_node *prev;

   bool retired;
   bson_error_t last_error;
} mongoc_topology_scanner_node_t;

typedef struct mongoc_topology_scanner {
   mongoc_async_t *async;
   mongoc_topology_scanner_node_t *nodes;
   bson_t ismaster_cmd;

   bson_t ismaster_cmd_with_handshake;
   bool handshake_ok_to_send;
   const char *appname;

   mongoc_topology_scanner_setup_err_cb_t setup_err_cb;
   mongoc_topology_scanner_cb_t cb;
   void *cb_data;
   bool in_progress;
   const mongoc_uri_t *uri;
   mongoc_async_cmd_setup_t setup;
   mongoc_stream_initiator_t initiator;
   void *initiator_context;
   bson_error_t error;

#ifdef MONGOC_ENABLE_SSL
   mongoc_ssl_opt_t *ssl_opts;
#endif

   mongoc_apm_callbacks_t apm_callbacks;
   void *apm_context;
} mongoc_topology_scanner_t;

mongoc_topology_scanner_t *
mongoc_topology_scanner_new (
   const mongoc_uri_t *uri,
   mongoc_topology_scanner_setup_err_cb_t setup_err_cb,
   mongoc_topology_scanner_cb_t cb,
   void *data);

void
mongoc_topology_scanner_destroy (mongoc_topology_scanner_t *ts);

void
mongoc_topology_scanner_add (mongoc_topology_scanner_t *ts,
                             const mongoc_host_list_t *host,
                             uint32_t id);

void
mongoc_topology_scanner_scan (mongoc_topology_scanner_t *ts,
                              uint32_t id,
                              int64_t timeout_msec);

void
mongoc_topology_scanner_node_retire (mongoc_topology_scanner_node_t *node);

void
mongoc_topology_scanner_node_disconnect (mongoc_topology_scanner_node_t *node,
                                         bool failed);

void
mongoc_topology_scanner_node_destroy (mongoc_topology_scanner_node_t *node,
                                      bool failed);

void
mongoc_topology_scanner_start (mongoc_topology_scanner_t *ts,
                               int64_t timeout_msec,
                               bool obey_cooldown);

void
mongoc_topology_scanner_work (mongoc_topology_scanner_t *ts);

void
_mongoc_topology_scanner_finish (mongoc_topology_scanner_t *ts);

void
mongoc_topology_scanner_get_error (mongoc_topology_scanner_t *ts,
                                   bson_error_t *error);

void
mongoc_topology_scanner_reset (mongoc_topology_scanner_t *ts);

bool
mongoc_topology_scanner_node_setup (mongoc_topology_scanner_node_t *node,
                                    bson_error_t *error);

mongoc_topology_scanner_node_t *
mongoc_topology_scanner_get_node (mongoc_topology_scanner_t *ts, uint32_t id);

bson_t *
_mongoc_topology_scanner_get_ismaster (mongoc_topology_scanner_t *ts);

bool
mongoc_topology_scanner_has_node_for_host (mongoc_topology_scanner_t *ts,
                                           mongoc_host_list_t *host);

void
mongoc_topology_scanner_set_stream_initiator (mongoc_topology_scanner_t *ts,
                                              mongoc_stream_initiator_t si,
                                              void *ctx);
bool
_mongoc_topology_scanner_set_appname (mongoc_topology_scanner_t *ts,
                                      const char *name);


#ifdef MONGOC_ENABLE_SSL
void
mongoc_topology_scanner_set_ssl_opts (mongoc_topology_scanner_t *ts,
                                      mongoc_ssl_opt_t *opts);
#endif

BSON_END_DECLS

#endif /* MONGOC_TOPOLOGY_SCANNER_PRIVATE_H */
