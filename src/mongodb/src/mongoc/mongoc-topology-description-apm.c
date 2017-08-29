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

#include "mongoc-topology-description-apm-private.h"
#include "mongoc-server-description-private.h"

/* Application Performance Monitoring for topology events, complies with the
 * SDAM Monitoring Spec:

https://github.com/mongodb/specifications/blob/master/source/server-discovery-and-monitoring/server-discovery-and-monitoring-monitoring.rst

 */

/* ServerOpeningEvent */
void
_mongoc_topology_description_monitor_server_opening (
   const mongoc_topology_description_t *td,
   mongoc_server_description_t *sd)
{
   if (td->apm_callbacks.server_opening && !sd->opened) {
      mongoc_apm_server_opening_t event;

      bson_oid_copy (&td->topology_id, &event.topology_id);
      event.host = &sd->host;
      event.context = td->apm_context;
      sd->opened = true;
      td->apm_callbacks.server_opening (&event);
   }
}

/* ServerDescriptionChangedEvent */
void
_mongoc_topology_description_monitor_server_changed (
   const mongoc_topology_description_t *td,
   const mongoc_server_description_t *prev_sd,
   const mongoc_server_description_t *new_sd)
{
   if (td->apm_callbacks.server_changed) {
      mongoc_apm_server_changed_t event;

      /* address is same in previous and new sd */
      bson_oid_copy (&td->topology_id, &event.topology_id);
      event.host = &new_sd->host;
      event.previous_description = prev_sd;
      event.new_description = new_sd;
      event.context = td->apm_context;
      td->apm_callbacks.server_changed (&event);
   }
}

/* ServerClosedEvent */
void
_mongoc_topology_description_monitor_server_closed (
   const mongoc_topology_description_t *td,
   const mongoc_server_description_t *sd)
{
   if (td->apm_callbacks.server_closed) {
      mongoc_apm_server_closed_t event;

      bson_oid_copy (&td->topology_id, &event.topology_id);
      event.host = &sd->host;
      event.context = td->apm_context;
      td->apm_callbacks.server_closed (&event);
   }
}


/* Send TopologyOpeningEvent when first called on this topology description.
 * td is not const: we set its "opened" field here */
void
_mongoc_topology_description_monitor_opening (mongoc_topology_description_t *td)
{
   mongoc_topology_description_t *prev_td = NULL;
   size_t i;
   mongoc_server_description_t *sd;

   if (td->opened) {
      return;
   }

   if (td->apm_callbacks.topology_changed) {
      /* prepare to call monitor_changed */
      prev_td = bson_malloc0 (sizeof (mongoc_topology_description_t));
      mongoc_topology_description_init (
         prev_td, MONGOC_TOPOLOGY_UNKNOWN, td->heartbeat_msec);
   }

   td->opened = true;

   if (td->apm_callbacks.topology_opening) {
      mongoc_apm_topology_opening_t event;

      bson_oid_copy (&td->topology_id, &event.topology_id);
      event.context = td->apm_context;
      td->apm_callbacks.topology_opening (&event);
   }

   if (td->apm_callbacks.topology_changed) {
      /* send initial description-changed event */
      _mongoc_topology_description_monitor_changed (prev_td, td);
   }

   for (i = 0; i < td->servers->items_len; i++) {
      sd = (mongoc_server_description_t *) mongoc_set_get_item (td->servers,
                                                                (int) i);

      _mongoc_topology_description_monitor_server_opening (td, sd);
   }

   if (prev_td) {
      mongoc_topology_description_destroy (prev_td);
      bson_free (prev_td);
   }
}

/* TopologyDescriptionChangedEvent */
void
_mongoc_topology_description_monitor_changed (
   const mongoc_topology_description_t *prev_td,
   const mongoc_topology_description_t *new_td)
{
   if (new_td->apm_callbacks.topology_changed) {
      mongoc_apm_topology_changed_t event;

      /* callbacks, context, and id are the same in previous and new td */
      bson_oid_copy (&new_td->topology_id, &event.topology_id);
      event.context = new_td->apm_context;
      event.previous_description = prev_td;
      event.new_description = new_td;

      new_td->apm_callbacks.topology_changed (&event);
   }
}

/* TopologyClosedEvent */
void
_mongoc_topology_description_monitor_closed (
   const mongoc_topology_description_t *td)
{
   if (td->apm_callbacks.topology_closed) {
      mongoc_apm_topology_closed_t event;

      bson_oid_copy (&td->topology_id, &event.topology_id);
      event.context = td->apm_context;
      td->apm_callbacks.topology_closed (&event);
   }
}
