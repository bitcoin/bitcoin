/*
 * Copyright 2016 MongoDB, Inc.
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

#ifndef MONGOC_TOPOLOGY_DESCRIPTION_APM_PRIVATE_H
#define MONGOC_TOPOLOGY_DESCRIPTION_APM_PRIVATE_H

#include <bson.h>
#include "mongoc-topology-description-private.h"

/* Application Performance Monitoring for topology events, complies with the
 * SDAM Monitoring Spec:

https://github.com/mongodb/specifications/blob/master/source/server-discovery-and-monitoring/server-discovery-and-monitoring-monitoring.rst

 */

void
_mongoc_topology_description_monitor_server_opening (
   const mongoc_topology_description_t *td,
   mongoc_server_description_t *sd);

void
_mongoc_topology_description_monitor_server_changed (
   const mongoc_topology_description_t *td,
   const mongoc_server_description_t *prev_sd,
   const mongoc_server_description_t *new_sd);

void
_mongoc_topology_description_monitor_server_closed (
   const mongoc_topology_description_t *td,
   const mongoc_server_description_t *sd);

/* td is not const: we set its "opened" field here */
void
_mongoc_topology_description_monitor_opening (
   mongoc_topology_description_t *td);

void
_mongoc_topology_description_monitor_changed (
   const mongoc_topology_description_t *prev_td,
   const mongoc_topology_description_t *new_td);

void
_mongoc_topology_description_monitor_closed (
   const mongoc_topology_description_t *td);

#endif /* MONGOC_TOPOLOGY_DESCRIPTION_APM_PRIVATE_H */
