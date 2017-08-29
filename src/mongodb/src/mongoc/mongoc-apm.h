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

#ifndef MONGOC_APM_H
#define MONGOC_APM_H

#if !defined(MONGOC_INSIDE) && !defined(MONGOC_COMPILATION)
#error "Only <mongoc.h> can be included directly."
#endif

#include <bson.h>

#include "mongoc-macros.h"
#include "mongoc-host-list.h"
#include "mongoc-server-description.h"
#include "mongoc-topology-description.h"

BSON_BEGIN_DECLS

/*
 * Application Performance Management (APM) interface, complies with two specs.
 * MongoDB's Command Monitoring Spec:
 *
 * https://github.com/mongodb/specifications/tree/master/source/command-monitoring
 *
 * MongoDB's Spec for Monitoring Server Discovery and Monitoring (SDAM) events:
 *
 * https://github.com/mongodb/specifications/tree/master/source/server-discovery-and-monitoring
 *
 */

/*
 * callbacks to receive APM events
 */

typedef struct _mongoc_apm_callbacks_t mongoc_apm_callbacks_t;


/*
 * command monitoring events
 */

typedef struct _mongoc_apm_command_started_t mongoc_apm_command_started_t;
typedef struct _mongoc_apm_command_succeeded_t mongoc_apm_command_succeeded_t;
typedef struct _mongoc_apm_command_failed_t mongoc_apm_command_failed_t;


/*
 * SDAM monitoring events
 */

typedef struct _mongoc_apm_server_changed_t mongoc_apm_server_changed_t;
typedef struct _mongoc_apm_server_opening_t mongoc_apm_server_opening_t;
typedef struct _mongoc_apm_server_closed_t mongoc_apm_server_closed_t;
typedef struct _mongoc_apm_topology_changed_t mongoc_apm_topology_changed_t;
typedef struct _mongoc_apm_topology_opening_t mongoc_apm_topology_opening_t;
typedef struct _mongoc_apm_topology_closed_t mongoc_apm_topology_closed_t;
typedef struct _mongoc_apm_server_heartbeat_started_t
   mongoc_apm_server_heartbeat_started_t;
typedef struct _mongoc_apm_server_heartbeat_succeeded_t
   mongoc_apm_server_heartbeat_succeeded_t;
typedef struct _mongoc_apm_server_heartbeat_failed_t
   mongoc_apm_server_heartbeat_failed_t;

/*
 * event field accessors
 */

/* command-started event fields */

MONGOC_EXPORT (const bson_t *)
mongoc_apm_command_started_get_command (
   const mongoc_apm_command_started_t *event);
MONGOC_EXPORT (const char *)
mongoc_apm_command_started_get_database_name (
   const mongoc_apm_command_started_t *event);
MONGOC_EXPORT (const char *)
mongoc_apm_command_started_get_command_name (
   const mongoc_apm_command_started_t *event);
MONGOC_EXPORT (int64_t)
mongoc_apm_command_started_get_request_id (
   const mongoc_apm_command_started_t *event);
MONGOC_EXPORT (int64_t)
mongoc_apm_command_started_get_operation_id (
   const mongoc_apm_command_started_t *event);
MONGOC_EXPORT (const mongoc_host_list_t *)
mongoc_apm_command_started_get_host (const mongoc_apm_command_started_t *event);
MONGOC_EXPORT (uint32_t)
mongoc_apm_command_started_get_server_id (
   const mongoc_apm_command_started_t *event);
MONGOC_EXPORT (void *)
mongoc_apm_command_started_get_context (
   const mongoc_apm_command_started_t *event);

/* command-succeeded event fields */

MONGOC_EXPORT (int64_t)
mongoc_apm_command_succeeded_get_duration (
   const mongoc_apm_command_succeeded_t *event);
MONGOC_EXPORT (const bson_t *)
mongoc_apm_command_succeeded_get_reply (
   const mongoc_apm_command_succeeded_t *event);
MONGOC_EXPORT (const char *)
mongoc_apm_command_succeeded_get_command_name (
   const mongoc_apm_command_succeeded_t *event);
MONGOC_EXPORT (int64_t)
mongoc_apm_command_succeeded_get_request_id (
   const mongoc_apm_command_succeeded_t *event);
MONGOC_EXPORT (int64_t)
mongoc_apm_command_succeeded_get_operation_id (
   const mongoc_apm_command_succeeded_t *event);
MONGOC_EXPORT (const mongoc_host_list_t *)
mongoc_apm_command_succeeded_get_host (
   const mongoc_apm_command_succeeded_t *event);
MONGOC_EXPORT (uint32_t)
mongoc_apm_command_succeeded_get_server_id (
   const mongoc_apm_command_succeeded_t *event);
MONGOC_EXPORT (void *)
mongoc_apm_command_succeeded_get_context (
   const mongoc_apm_command_succeeded_t *event);

/* command-failed event fields */

MONGOC_EXPORT (int64_t)
mongoc_apm_command_failed_get_duration (
   const mongoc_apm_command_failed_t *event);
MONGOC_EXPORT (const char *)
mongoc_apm_command_failed_get_command_name (
   const mongoc_apm_command_failed_t *event);
/* retrieve the error by filling out the passed-in "error" struct */
MONGOC_EXPORT (void)
mongoc_apm_command_failed_get_error (const mongoc_apm_command_failed_t *event,
                                     bson_error_t *error);
MONGOC_EXPORT (int64_t)
mongoc_apm_command_failed_get_request_id (
   const mongoc_apm_command_failed_t *event);
MONGOC_EXPORT (int64_t)
mongoc_apm_command_failed_get_operation_id (
   const mongoc_apm_command_failed_t *event);
MONGOC_EXPORT (const mongoc_host_list_t *)
mongoc_apm_command_failed_get_host (const mongoc_apm_command_failed_t *event);
MONGOC_EXPORT (uint32_t)
mongoc_apm_command_failed_get_server_id (
   const mongoc_apm_command_failed_t *event);
MONGOC_EXPORT (void *)
mongoc_apm_command_failed_get_context (
   const mongoc_apm_command_failed_t *event);

/* server-changed event fields */

MONGOC_EXPORT (const mongoc_host_list_t *)
mongoc_apm_server_changed_get_host (const mongoc_apm_server_changed_t *event);
MONGOC_EXPORT (void)
mongoc_apm_server_changed_get_topology_id (
   const mongoc_apm_server_changed_t *event, bson_oid_t *topology_id);
MONGOC_EXPORT (const mongoc_server_description_t *)
mongoc_apm_server_changed_get_previous_description (
   const mongoc_apm_server_changed_t *event);
MONGOC_EXPORT (const mongoc_server_description_t *)
mongoc_apm_server_changed_get_new_description (
   const mongoc_apm_server_changed_t *event);
MONGOC_EXPORT (void *)
mongoc_apm_server_changed_get_context (
   const mongoc_apm_server_changed_t *event);

/* server-opening event fields */

MONGOC_EXPORT (const mongoc_host_list_t *)
mongoc_apm_server_opening_get_host (const mongoc_apm_server_opening_t *event);
MONGOC_EXPORT (void)
mongoc_apm_server_opening_get_topology_id (
   const mongoc_apm_server_opening_t *event, bson_oid_t *topology_id);
MONGOC_EXPORT (void *)
mongoc_apm_server_opening_get_context (
   const mongoc_apm_server_opening_t *event);

/* server-closed event fields */

MONGOC_EXPORT (const mongoc_host_list_t *)
mongoc_apm_server_closed_get_host (const mongoc_apm_server_closed_t *event);
MONGOC_EXPORT (void)
mongoc_apm_server_closed_get_topology_id (
   const mongoc_apm_server_closed_t *event, bson_oid_t *topology_id);
MONGOC_EXPORT (void *)
mongoc_apm_server_closed_get_context (const mongoc_apm_server_closed_t *event);

/* topology-changed event fields */

MONGOC_EXPORT (void)
mongoc_apm_topology_changed_get_topology_id (
   const mongoc_apm_topology_changed_t *event, bson_oid_t *topology_id);
MONGOC_EXPORT (const mongoc_topology_description_t *)
mongoc_apm_topology_changed_get_previous_description (
   const mongoc_apm_topology_changed_t *event);
MONGOC_EXPORT (const mongoc_topology_description_t *)
mongoc_apm_topology_changed_get_new_description (
   const mongoc_apm_topology_changed_t *event);
MONGOC_EXPORT (void *)
mongoc_apm_topology_changed_get_context (
   const mongoc_apm_topology_changed_t *event);

/* topology-opening event field */

MONGOC_EXPORT (void)
mongoc_apm_topology_opening_get_topology_id (
   const mongoc_apm_topology_opening_t *event, bson_oid_t *topology_id);
MONGOC_EXPORT (void *)
mongoc_apm_topology_opening_get_context (
   const mongoc_apm_topology_opening_t *event);

/* topology-closed event field */

MONGOC_EXPORT (void)
mongoc_apm_topology_closed_get_topology_id (
   const mongoc_apm_topology_closed_t *event, bson_oid_t *topology_id);
MONGOC_EXPORT (void *)
mongoc_apm_topology_closed_get_context (
   const mongoc_apm_topology_closed_t *event);

/* heartbeat-started event field */

MONGOC_EXPORT (const mongoc_host_list_t *)
mongoc_apm_server_heartbeat_started_get_host (
   const mongoc_apm_server_heartbeat_started_t *event);
MONGOC_EXPORT (void *)
mongoc_apm_server_heartbeat_started_get_context (
   const mongoc_apm_server_heartbeat_started_t *event);

/* heartbeat-succeeded event fields */

MONGOC_EXPORT (int64_t)
mongoc_apm_server_heartbeat_succeeded_get_duration (
   const mongoc_apm_server_heartbeat_succeeded_t *event);
MONGOC_EXPORT (const bson_t *)
mongoc_apm_server_heartbeat_succeeded_get_reply (
   const mongoc_apm_server_heartbeat_succeeded_t *event);
MONGOC_EXPORT (const mongoc_host_list_t *)
mongoc_apm_server_heartbeat_succeeded_get_host (
   const mongoc_apm_server_heartbeat_succeeded_t *event);
MONGOC_EXPORT (void *)
mongoc_apm_server_heartbeat_succeeded_get_context (
   const mongoc_apm_server_heartbeat_succeeded_t *event);

/* heartbeat-failed event fields */

MONGOC_EXPORT (int64_t)
mongoc_apm_server_heartbeat_failed_get_duration (
   const mongoc_apm_server_heartbeat_failed_t *event);
MONGOC_EXPORT (void)
mongoc_apm_server_heartbeat_failed_get_error (
   const mongoc_apm_server_heartbeat_failed_t *event, bson_error_t *error);
MONGOC_EXPORT (const mongoc_host_list_t *)
mongoc_apm_server_heartbeat_failed_get_host (
   const mongoc_apm_server_heartbeat_failed_t *event);
MONGOC_EXPORT (void *)
mongoc_apm_server_heartbeat_failed_get_context (
   const mongoc_apm_server_heartbeat_failed_t *event);


/*
 * callbacks
 */

typedef void (*mongoc_apm_command_started_cb_t) (
   const mongoc_apm_command_started_t *event);
typedef void (*mongoc_apm_command_succeeded_cb_t) (
   const mongoc_apm_command_succeeded_t *event);
typedef void (*mongoc_apm_command_failed_cb_t) (
   const mongoc_apm_command_failed_t *event);
typedef void (*mongoc_apm_server_changed_cb_t) (
   const mongoc_apm_server_changed_t *event);
typedef void (*mongoc_apm_server_opening_cb_t) (
   const mongoc_apm_server_opening_t *event);
typedef void (*mongoc_apm_server_closed_cb_t) (
   const mongoc_apm_server_closed_t *event);
typedef void (*mongoc_apm_topology_changed_cb_t) (
   const mongoc_apm_topology_changed_t *event);
typedef void (*mongoc_apm_topology_opening_cb_t) (
   const mongoc_apm_topology_opening_t *event);
typedef void (*mongoc_apm_topology_closed_cb_t) (
   const mongoc_apm_topology_closed_t *event);
typedef void (*mongoc_apm_server_heartbeat_started_cb_t) (
   const mongoc_apm_server_heartbeat_started_t *event);
typedef void (*mongoc_apm_server_heartbeat_succeeded_cb_t) (
   const mongoc_apm_server_heartbeat_succeeded_t *event);
typedef void (*mongoc_apm_server_heartbeat_failed_cb_t) (
   const mongoc_apm_server_heartbeat_failed_t *event);

/*
 * registering callbacks
 */

MONGOC_EXPORT (mongoc_apm_callbacks_t *)
mongoc_apm_callbacks_new (void);
MONGOC_EXPORT (void)
mongoc_apm_callbacks_destroy (mongoc_apm_callbacks_t *callbacks);
MONGOC_EXPORT (void)
mongoc_apm_set_command_started_cb (mongoc_apm_callbacks_t *callbacks,
                                   mongoc_apm_command_started_cb_t cb);
MONGOC_EXPORT (void)
mongoc_apm_set_command_succeeded_cb (mongoc_apm_callbacks_t *callbacks,
                                     mongoc_apm_command_succeeded_cb_t cb);
MONGOC_EXPORT (void)
mongoc_apm_set_command_failed_cb (mongoc_apm_callbacks_t *callbacks,
                                  mongoc_apm_command_failed_cb_t cb);
MONGOC_EXPORT (void)
mongoc_apm_set_server_changed_cb (mongoc_apm_callbacks_t *callbacks,
                                  mongoc_apm_server_changed_cb_t cb);
MONGOC_EXPORT (void)
mongoc_apm_set_server_opening_cb (mongoc_apm_callbacks_t *callbacks,
                                  mongoc_apm_server_opening_cb_t cb);
MONGOC_EXPORT (void)
mongoc_apm_set_server_closed_cb (mongoc_apm_callbacks_t *callbacks,
                                 mongoc_apm_server_closed_cb_t cb);
MONGOC_EXPORT (void)
mongoc_apm_set_topology_changed_cb (mongoc_apm_callbacks_t *callbacks,
                                    mongoc_apm_topology_changed_cb_t cb);
MONGOC_EXPORT (void)
mongoc_apm_set_topology_opening_cb (mongoc_apm_callbacks_t *callbacks,
                                    mongoc_apm_topology_opening_cb_t cb);
MONGOC_EXPORT (void)
mongoc_apm_set_topology_closed_cb (mongoc_apm_callbacks_t *callbacks,
                                   mongoc_apm_topology_closed_cb_t cb);
MONGOC_EXPORT (void)
mongoc_apm_set_server_heartbeat_started_cb (
   mongoc_apm_callbacks_t *callbacks,
   mongoc_apm_server_heartbeat_started_cb_t cb);
MONGOC_EXPORT (void)
mongoc_apm_set_server_heartbeat_succeeded_cb (
   mongoc_apm_callbacks_t *callbacks,
   mongoc_apm_server_heartbeat_succeeded_cb_t cb);
MONGOC_EXPORT (void)
mongoc_apm_set_server_heartbeat_failed_cb (
   mongoc_apm_callbacks_t *callbacks,
   mongoc_apm_server_heartbeat_failed_cb_t cb);
BSON_END_DECLS

#endif /* MONGOC_APM_H */
