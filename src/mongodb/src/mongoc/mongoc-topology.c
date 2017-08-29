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

#include "mongoc-config.h"

#include "mongoc-handshake.h"
#include "mongoc-handshake-private.h"

#include "mongoc-error.h"
#include "mongoc-log.h"
#include "mongoc-topology-private.h"
#include "mongoc-topology-description-apm-private.h"
#include "mongoc-client-private.h"
#include "mongoc-uri-private.h"
#include "mongoc-util-private.h"

#include "utlist.h"

static void
_mongoc_topology_background_thread_stop (mongoc_topology_t *topology);

static void
_mongoc_topology_request_scan (mongoc_topology_t *topology);

static bool
_mongoc_topology_reconcile_add_nodes (void *item, void *ctx)
{
   mongoc_server_description_t *sd = item;
   mongoc_topology_t *topology = (mongoc_topology_t *) ctx;
   mongoc_topology_scanner_t *scanner = topology->scanner;

   /* quickly search by id, then check if a node for this host was retired in
    * this scan. */
   if (!mongoc_topology_scanner_get_node (scanner, sd->id) &&
       !mongoc_topology_scanner_has_node_for_host (scanner, &sd->host)) {
      mongoc_topology_scanner_add (scanner, &sd->host, sd->id);
      mongoc_topology_scanner_scan (
         scanner, sd->id, topology->connect_timeout_msec);
   }

   return true;
}

void
mongoc_topology_reconcile (mongoc_topology_t *topology)
{
   mongoc_topology_scanner_node_t *ele, *tmp;
   mongoc_topology_description_t *description;
   mongoc_topology_scanner_t *scanner;

   description = &topology->description;
   scanner = topology->scanner;

   /* Add newly discovered nodes */
   mongoc_set_for_each (
      description->servers, _mongoc_topology_reconcile_add_nodes, topology);

   /* Remove removed nodes */
   DL_FOREACH_SAFE (scanner->nodes, ele, tmp)
   {
      if (!mongoc_topology_description_server_by_id (
             description, ele->id, NULL)) {
         mongoc_topology_scanner_node_retire (ele);
      }
   }
}


/* call this while already holding the lock */
static bool
_mongoc_topology_update_no_lock (uint32_t id,
                                 const bson_t *ismaster_response,
                                 int64_t rtt_msec,
                                 mongoc_topology_t *topology,
                                 const bson_error_t *error /* IN */)
{
   mongoc_topology_description_handle_ismaster (
      &topology->description, id, ismaster_response, rtt_msec, error);

   /* The processing of the ismaster results above may have added/removed
    * server descriptions. We need to reconcile that with our monitoring agents
    */
   mongoc_topology_reconcile (topology);

   /* return false if server removed from topology */
   return mongoc_topology_description_server_by_id (
             &topology->description, id, NULL) != NULL;
}


/*
 *-------------------------------------------------------------------------
 *
 * _mongoc_topology_scanner_setup_err_cb --
 *
 *       Callback method to handle errors during topology scanner node
 *       setup, typically DNS or SSL errors.
 *
 *-------------------------------------------------------------------------
 */

void
_mongoc_topology_scanner_setup_err_cb (uint32_t id,
                                       void *data,
                                       const bson_error_t *error /* IN */)
{
   mongoc_topology_t *topology;

   BSON_ASSERT (data);

   topology = (mongoc_topology_t *) data;

   mongoc_topology_description_handle_ismaster (&topology->description,
                                                id,
                                                NULL /* ismaster reply */,
                                                -1 /* rtt_msec */,
                                                error);
}


/*
 *-------------------------------------------------------------------------
 *
 * _mongoc_topology_scanner_cb --
 *
 *       Callback method to handle ismaster responses received by async
 *       command objects.
 *
 *       NOTE: This method locks the given topology's mutex.
 *
 *-------------------------------------------------------------------------
 */

void
_mongoc_topology_scanner_cb (uint32_t id,
                             const bson_t *ismaster_response,
                             int64_t rtt_msec,
                             void *data,
                             const bson_error_t *error /* IN */)
{
   mongoc_topology_t *topology;
   mongoc_server_description_t *sd;

   BSON_ASSERT (data);

   topology = (mongoc_topology_t *) data;

   mongoc_mutex_lock (&topology->mutex);
   sd = mongoc_topology_description_server_by_id (
      &topology->description, id, NULL);

   /* Server Discovery and Monitoring Spec: "Once a server is connected, the
    * client MUST change its type to Unknown only after it has retried the
    * server once." */
   if (!ismaster_response && sd && sd->type != MONGOC_SERVER_UNKNOWN) {
      _mongoc_topology_update_no_lock (
         id, ismaster_response, rtt_msec, topology, error);

      /* add another ismaster call to the current scan - the scan continues
       * until all commands are done */
      mongoc_topology_scanner_scan (
         topology->scanner, sd->id, topology->connect_timeout_msec);
   } else {
      _mongoc_topology_update_no_lock (
         id, ismaster_response, rtt_msec, topology, error);

      mongoc_cond_broadcast (&topology->cond_client);
   }

   mongoc_mutex_unlock (&topology->mutex);
}

/*
 *-------------------------------------------------------------------------
 *
 * mongoc_topology_new --
 *
 *       Creates and returns a new topology object.
 *
 * Returns:
 *       A new topology object.
 *
 * Side effects:
 *       None.
 *
 *-------------------------------------------------------------------------
 */
mongoc_topology_t *
mongoc_topology_new (const mongoc_uri_t *uri, bool single_threaded)
{
   int64_t heartbeat_default;
   int64_t heartbeat;
   mongoc_topology_t *topology;
   mongoc_topology_description_type_t init_type;
   uint32_t id;
   const mongoc_host_list_t *hl;

   BSON_ASSERT (uri);

   topology = (mongoc_topology_t *) bson_malloc0 (sizeof *topology);

   /*
    * Not ideal, but there's no great way to do this.
    * Base on the URI, we assume:
    *   - if we've got a replicaSet name, initialize to RS_NO_PRIMARY
    *   - otherwise, if the seed list has a single host, initialize to SINGLE
    *   - everything else gets initialized to UNKNOWN
    */
   if (mongoc_uri_get_replica_set (uri)) {
      init_type = MONGOC_TOPOLOGY_RS_NO_PRIMARY;
   } else {
      hl = mongoc_uri_get_hosts (uri);
      if (hl->next) {
         init_type = MONGOC_TOPOLOGY_UNKNOWN;
      } else {
         init_type = MONGOC_TOPOLOGY_SINGLE;
      }
   }

   heartbeat_default =
      single_threaded ? MONGOC_TOPOLOGY_HEARTBEAT_FREQUENCY_MS_SINGLE_THREADED
                      : MONGOC_TOPOLOGY_HEARTBEAT_FREQUENCY_MS_MULTI_THREADED;

   heartbeat = mongoc_uri_get_option_as_int32 (
      uri, MONGOC_URI_HEARTBEATFREQUENCYMS, heartbeat_default);

   mongoc_topology_description_init (
      &topology->description, init_type, heartbeat);

   topology->description.set_name =
      bson_strdup (mongoc_uri_get_replica_set (uri));

   topology->uri = mongoc_uri_copy (uri);
   topology->scanner_state = MONGOC_TOPOLOGY_SCANNER_OFF;
   topology->scanner =
      mongoc_topology_scanner_new (topology->uri,
                                   _mongoc_topology_scanner_setup_err_cb,
                                   _mongoc_topology_scanner_cb,
                                   topology);

   topology->single_threaded = single_threaded;
   if (single_threaded) {
      /* Server Selection Spec:
       *
       *   "Single-threaded drivers MUST provide a "serverSelectionTryOnce"
       *   mode, in which the driver scans the topology exactly once after
       *   server selection fails, then either selects a server or raises an
       *   error.
       *
       *   "The serverSelectionTryOnce option MUST be true by default."
       */
      topology->server_selection_try_once = mongoc_uri_get_option_as_bool (
         uri, MONGOC_URI_SERVERSELECTIONTRYONCE, true);
   } else {
      topology->server_selection_try_once = false;
   }

   topology->server_selection_timeout_msec = mongoc_uri_get_option_as_int32 (
      topology->uri,
      MONGOC_URI_SERVERSELECTIONTIMEOUTMS,
      MONGOC_TOPOLOGY_SERVER_SELECTION_TIMEOUT_MS);

   topology->local_threshold_msec =
      mongoc_uri_get_local_threshold_option (topology->uri);

   /* Total time allowed to check a server is connectTimeoutMS.
    * Server Discovery And Monitoring Spec:
    *
    *   "The socket used to check a server MUST use the same connectTimeoutMS as
    *   regular sockets. Multi-threaded clients SHOULD set monitoring sockets'
    *   socketTimeoutMS to the connectTimeoutMS."
    */
   topology->connect_timeout_msec =
      mongoc_uri_get_option_as_int32 (topology->uri,
                                      MONGOC_URI_CONNECTTIMEOUTMS,
                                      MONGOC_DEFAULT_CONNECTTIMEOUTMS);

   mongoc_mutex_init (&topology->mutex);
   mongoc_cond_init (&topology->cond_client);
   mongoc_cond_init (&topology->cond_server);

   for (hl = mongoc_uri_get_hosts (uri); hl; hl = hl->next) {
      mongoc_topology_description_add_server (
         &topology->description, hl->host_and_port, &id);
      mongoc_topology_scanner_add (topology->scanner, hl, id);
   }

   return topology;
}
/*
 *-------------------------------------------------------------------------
 *
 * mongoc_topology_set_apm_callbacks --
 *
 *       Set Application Performance Monitoring callbacks.
 *
 *-------------------------------------------------------------------------
 */
void
mongoc_topology_set_apm_callbacks (mongoc_topology_t *topology,
                                   mongoc_apm_callbacks_t *callbacks,
                                   void *context)
{
   if (callbacks) {
      memcpy (&topology->description.apm_callbacks,
              callbacks,
              sizeof (mongoc_apm_callbacks_t));
      memcpy (&topology->scanner->apm_callbacks,
              callbacks,
              sizeof (mongoc_apm_callbacks_t));
   }

   topology->description.apm_context = context;
   topology->scanner->apm_context = context;
}

/*
 *-------------------------------------------------------------------------
 *
 * mongoc_topology_destroy --
 *
 *       Free the memory associated with this topology object.
 *
 * Returns:
 *       None.
 *
 * Side effects:
 *       @topology will be cleaned up.
 *
 *-------------------------------------------------------------------------
 */
void
mongoc_topology_destroy (mongoc_topology_t *topology)
{
   if (!topology) {
      return;
   }

   _mongoc_topology_background_thread_stop (topology);
   _mongoc_topology_description_monitor_closed (&topology->description);

   mongoc_uri_destroy (topology->uri);
   mongoc_topology_description_destroy (&topology->description);
   mongoc_topology_scanner_destroy (topology->scanner);
   mongoc_cond_destroy (&topology->cond_client);
   mongoc_cond_destroy (&topology->cond_server);
   mongoc_mutex_destroy (&topology->mutex);

   bson_free (topology);
}


/*
 *--------------------------------------------------------------------------
 *
 * _mongoc_topology_do_blocking_scan --
 *
 *       Monitoring entry for single-threaded use case. Assumes the caller
 *       has checked that it's the right time to scan.
 *
 *--------------------------------------------------------------------------
 */
static void
_mongoc_topology_do_blocking_scan (mongoc_topology_t *topology,
                                   bson_error_t *error)
{
   mongoc_topology_scanner_t *scanner;

   topology->scanner_state = MONGOC_TOPOLOGY_SCANNER_SINGLE_THREADED;

   _mongoc_handshake_freeze ();

   scanner = topology->scanner;
   mongoc_topology_scanner_start (
      scanner, (int32_t) topology->connect_timeout_msec, true);

   mongoc_topology_scanner_work (topology->scanner);

   mongoc_mutex_lock (&topology->mutex);
   _mongoc_topology_scanner_finish (scanner);
   mongoc_topology_scanner_get_error (scanner, error);

   /* "retired" nodes can be checked again in the next scan */
   mongoc_topology_scanner_reset (scanner);
   topology->last_scan = bson_get_monotonic_time ();
   topology->stale = false;
   mongoc_mutex_unlock (&topology->mutex);
}


bool
mongoc_topology_compatible (const mongoc_topology_description_t *td,
                            const mongoc_read_prefs_t *read_prefs,
                            bson_error_t *error)
{
   int64_t max_staleness_seconds;
   int32_t max_wire_version;

   if (td->compatibility_error.code) {
      memcpy (error, &td->compatibility_error, sizeof (bson_error_t));
      return false;
   }

   if (!read_prefs) {
      /* NULL means read preference Primary */
      return true;
   }

   max_staleness_seconds =
      mongoc_read_prefs_get_max_staleness_seconds (read_prefs);

   if (max_staleness_seconds != MONGOC_NO_MAX_STALENESS) {
      max_wire_version =
         mongoc_topology_description_lowest_max_wire_version (td);

      if (max_wire_version < WIRE_VERSION_MAX_STALENESS) {
         bson_set_error (error,
                         MONGOC_ERROR_COMMAND,
                         MONGOC_ERROR_PROTOCOL_BAD_WIRE_VERSION,
                         "Not all servers support maxStalenessSeconds");
         return false;
      }

      /* shouldn't happen if we've properly enforced wire version */
      if (!mongoc_topology_description_all_sds_have_write_date (td)) {
         bson_set_error (error,
                         MONGOC_ERROR_COMMAND,
                         MONGOC_ERROR_PROTOCOL_BAD_WIRE_VERSION,
                         "Not all servers have lastWriteDate");
         return false;
      }

      if (!_mongoc_topology_description_validate_max_staleness (
             td, max_staleness_seconds, error)) {
         return false;
      }
   }

   return true;
}


static void
_mongoc_server_selection_error (const char *msg,
                                const bson_error_t *scanner_error,
                                bson_error_t *error)
{
   if (scanner_error && scanner_error->code) {
      bson_set_error (error,
                      MONGOC_ERROR_SERVER_SELECTION,
                      MONGOC_ERROR_SERVER_SELECTION_FAILURE,
                      "%s: %s",
                      msg,
                      scanner_error->message);
   } else {
      bson_set_error (error,
                      MONGOC_ERROR_SERVER_SELECTION,
                      MONGOC_ERROR_SERVER_SELECTION_FAILURE,
                      "%s",
                      msg);
   }
}

/*
 *-------------------------------------------------------------------------
 *
 * mongoc_topology_select --
 *
 *       Selects a server description for an operation based on @optype
 *       and @read_prefs.
 *
 *       NOTE: this method returns a copy of the original server
 *       description. Callers must own and clean up this copy.
 *
 *       NOTE: this method locks and unlocks @topology's mutex.
 *
 * Parameters:
 *       @topology: The topology.
 *       @optype: Whether we are selecting for a read or write operation.
 *       @read_prefs: Required, the read preferences for the command.
 *       @error: Required, out pointer for error info.
 *
 * Returns:
 *       A mongoc_server_description_t, or NULL on failure, in which case
 *       @error will be set.
 *
 * Side effects:
 *       @error may be set.
 *
 *-------------------------------------------------------------------------
 */
mongoc_server_description_t *
mongoc_topology_select (mongoc_topology_t *topology,
                        mongoc_ss_optype_t optype,
                        const mongoc_read_prefs_t *read_prefs,
                        bson_error_t *error)
{
   uint32_t server_id =
      mongoc_topology_select_server_id (topology, optype, read_prefs, error);

   if (server_id) {
      /* new copy of the server description */
      return mongoc_topology_server_by_id (topology, server_id, error);
   } else {
      return NULL;
   }
}

/*
 *-------------------------------------------------------------------------
 *
 * mongoc_topology_select_server_id --
 *
 *       Alternative to mongoc_topology_select when you only need the id.
 *
 * Returns:
 *       A server id, or 0 on failure, in which case @error will be set.
 *
 *-------------------------------------------------------------------------
 */
uint32_t
mongoc_topology_select_server_id (mongoc_topology_t *topology,
                                  mongoc_ss_optype_t optype,
                                  const mongoc_read_prefs_t *read_prefs,
                                  bson_error_t *error)
{
   static const char *timeout_msg =
      "No suitable servers found: `serverSelectionTimeoutMS` expired";
   int r;
   int64_t local_threshold_ms;
   mongoc_server_description_t *selected_server = NULL;
   bool try_once;
   int64_t sleep_usec;
   bool tried_once;
   bson_error_t scanner_error = {0};
   int64_t heartbeat_msec;
   uint32_t server_id;

   /* These names come from the Server Selection Spec pseudocode */
   int64_t loop_start;  /* when we entered this function */
   int64_t loop_end;    /* when we last completed a loop (single-threaded) */
   int64_t scan_ready;  /* the soonest we can do a blocking scan */
   int64_t next_update; /* the latest we must do a blocking scan */
   int64_t expire_at;   /* when server selection timeout expires */

   BSON_ASSERT (topology);

   heartbeat_msec = topology->description.heartbeat_msec;
   local_threshold_ms = topology->local_threshold_msec;
   try_once = topology->server_selection_try_once;
   loop_start = loop_end = bson_get_monotonic_time ();
   expire_at =
      loop_start + ((int64_t) topology->server_selection_timeout_msec * 1000);

   if (topology->single_threaded) {
      _mongoc_topology_description_monitor_opening (&topology->description);

      tried_once = false;
      next_update = topology->last_scan + heartbeat_msec * 1000;
      if (next_update < loop_start) {
         /* we must scan now */
         topology->stale = true;
      }

      /* until we find a server or time out */
      for (;;) {
         if (topology->stale) {
            /* how soon are we allowed to scan? */
            scan_ready = topology->last_scan +
                         MONGOC_TOPOLOGY_MIN_HEARTBEAT_FREQUENCY_MS * 1000;

            if (scan_ready > expire_at && !try_once) {
               /* selection timeout will expire before min heartbeat passes */
               _mongoc_server_selection_error (
                  "No suitable servers found: "
                  "`serverselectiontimeoutms` timed out",
                  &scanner_error,
                  error);

               topology->stale = true;
               return 0;
            }

            sleep_usec = scan_ready - loop_end;
            if (sleep_usec > 0) {
               _mongoc_usleep (sleep_usec);
            }

            /* takes up to connectTimeoutMS. sets "last_scan", clears "stale" */
            _mongoc_topology_do_blocking_scan (topology, &scanner_error);
            loop_end = topology->last_scan;
            tried_once = true;
         }

         if (!mongoc_topology_compatible (
                &topology->description, read_prefs, error)) {
            return 0;
         }

         selected_server = mongoc_topology_description_select (
            &topology->description, optype, read_prefs, local_threshold_ms);

         if (selected_server) {
            return selected_server->id;
         }

         topology->stale = true;

         if (try_once) {
            if (tried_once) {
               _mongoc_server_selection_error (
                  "No suitable servers found (`serverSelectionTryOnce` set)",
                  &scanner_error,
                  error);

               return 0;
            }
         } else {
            loop_end = bson_get_monotonic_time ();

            if (loop_end > expire_at) {
               /* no time left in server_selection_timeout_msec */
               _mongoc_server_selection_error (
                  timeout_msg, &scanner_error, error);

               return 0;
            }
         }
      }
   }

   /* With background thread */
   /* we break out when we've found a server or timed out */
   for (;;) {
      mongoc_mutex_lock (&topology->mutex);

      if (!mongoc_topology_compatible (
             &topology->description, read_prefs, error)) {
         mongoc_mutex_unlock (&topology->mutex);
         return 0;
      }

      selected_server = mongoc_topology_description_select (
         &topology->description, optype, read_prefs, local_threshold_ms);

      if (!selected_server) {
         _mongoc_topology_request_scan (topology);

         r = mongoc_cond_timedwait (&topology->cond_client,
                                    &topology->mutex,
                                    (expire_at - loop_start) / 1000);

         mongoc_topology_scanner_get_error (topology->scanner, &scanner_error);
         mongoc_mutex_unlock (&topology->mutex);

#ifdef _WIN32
         if (r == WSAETIMEDOUT) {
#else
         if (r == ETIMEDOUT) {
#endif
            /* handle timeouts */
            _mongoc_server_selection_error (timeout_msg, &scanner_error, error);

            return 0;
         } else if (r) {
            bson_set_error (error,
                            MONGOC_ERROR_SERVER_SELECTION,
                            MONGOC_ERROR_SERVER_SELECTION_FAILURE,
                            "Unknown error '%d' received while waiting on "
                            "thread condition",
                            r);
            return 0;
         }

         loop_start = bson_get_monotonic_time ();

         if (loop_start > expire_at) {
            _mongoc_server_selection_error (timeout_msg, &scanner_error, error);

            return 0;
         }
      } else {
         server_id = selected_server->id;
         mongoc_mutex_unlock (&topology->mutex);
         return server_id;
      }
   }
}

/*
 *-------------------------------------------------------------------------
 *
 * mongoc_topology_server_by_id --
 *
 *      Get the server description for @id, if that server is present
 *      in @description. Otherwise, return NULL and fill out the optional
 *      @error.
 *
 *      NOTE: this method returns a copy of the original server
 *      description. Callers must own and clean up this copy.
 *
 *      NOTE: this method locks and unlocks @topology's mutex.
 *
 * Returns:
 *      A mongoc_server_description_t, or NULL.
 *
 * Side effects:
 *      Fills out optional @error if server not found.
 *
 *-------------------------------------------------------------------------
 */

mongoc_server_description_t *
mongoc_topology_server_by_id (mongoc_topology_t *topology,
                              uint32_t id,
                              bson_error_t *error)
{
   mongoc_server_description_t *sd;

   mongoc_mutex_lock (&topology->mutex);

   sd = mongoc_server_description_new_copy (
      mongoc_topology_description_server_by_id (
         &topology->description, id, error));

   mongoc_mutex_unlock (&topology->mutex);

   return sd;
}

/*
 *-------------------------------------------------------------------------
 *
 * mongoc_topology_host_by_id --
 *
 *      Copy the mongoc_host_list_t for @id, if that server is present
 *      in @description. Otherwise, return NULL and fill out the optional
 *      @error.
 *
 *      NOTE: this method returns a copy of the original mongoc_host_list_t.
 *      Callers must own and clean up this copy.
 *
 *      NOTE: this method locks and unlocks @topology's mutex.
 *
 * Returns:
 *      A mongoc_host_list_t, or NULL.
 *
 * Side effects:
 *      Fills out optional @error if server not found.
 *
 *-------------------------------------------------------------------------
 */

mongoc_host_list_t *
_mongoc_topology_host_by_id (mongoc_topology_t *topology,
                             uint32_t id,
                             bson_error_t *error)
{
   mongoc_server_description_t *sd;
   mongoc_host_list_t *host = NULL;

   mongoc_mutex_lock (&topology->mutex);

   /* not a copy - direct pointer into topology description data */
   sd = mongoc_topology_description_server_by_id (
      &topology->description, id, error);

   if (sd) {
      host = bson_malloc0 (sizeof (mongoc_host_list_t));
      memcpy (host, &sd->host, sizeof (mongoc_host_list_t));
   }

   mongoc_mutex_unlock (&topology->mutex);

   return host;
}

/*
 *--------------------------------------------------------------------------
 *
 * _mongoc_topology_request_scan --
 *
 *       Non-locking variant
 *
 *--------------------------------------------------------------------------
 */

static void
_mongoc_topology_request_scan (mongoc_topology_t *topology)
{
   topology->scan_requested = true;

   mongoc_cond_signal (&topology->cond_server);
}

/*
 *--------------------------------------------------------------------------
 *
 * mongoc_topology_invalidate_server --
 *
 *      Invalidate the given server after receiving a network error in
 *      another part of the client.
 *
 *      NOTE: this method uses @topology's mutex.
 *
 *--------------------------------------------------------------------------
 */
void
mongoc_topology_invalidate_server (mongoc_topology_t *topology,
                                   uint32_t id,
                                   const bson_error_t *error)
{
   BSON_ASSERT (error);

   mongoc_mutex_lock (&topology->mutex);
   mongoc_topology_description_invalidate_server (
      &topology->description, id, error);
   mongoc_mutex_unlock (&topology->mutex);
}

/*
 *--------------------------------------------------------------------------
 *
 * _mongoc_topology_update_from_handshake --
 *
 *      A client opens a new connection and calls ismaster on it when it
 *      detects a closed connection in _mongoc_cluster_check_interval, or if
 *      mongoc_client_pool_pop creates a new client. Update the topology
 *      description from the ismaster response.
 *
 *      NOTE: this method uses @topology's mutex.
 *
 * Returns:
 *      false if the server was removed from the topology
 *--------------------------------------------------------------------------
 */
bool
_mongoc_topology_update_from_handshake (mongoc_topology_t *topology,
                                        const mongoc_server_description_t *sd)
{
   bool has_server;

   BSON_ASSERT (topology);
   BSON_ASSERT (sd);

   mongoc_mutex_lock (&topology->mutex);

   mongoc_topology_description_handle_ismaster (&topology->description,
                                                sd->id,
                                                &sd->last_is_master,
                                                sd->round_trip_time_msec,
                                                NULL);

   /* return false if server was removed from topology */
   has_server = mongoc_topology_description_server_by_id (
                   &topology->description, sd->id, NULL) != NULL;

   /* if pooled, wake threads waiting in mongoc_topology_server_by_id */
   mongoc_cond_broadcast (&topology->cond_client);
   mongoc_mutex_unlock (&topology->mutex);

   return has_server;
}

/*
 *--------------------------------------------------------------------------
 *
 * mongoc_topology_server_timestamp --
 *
 *      Return the topology's scanner's timestamp for the given server,
 *      or -1 if there is no scanner node for the given server.
 *
 *      NOTE: this method uses @topology's mutex.
 *
 * Returns:
 *      Timestamp, or -1
 *
 *--------------------------------------------------------------------------
 */
int64_t
mongoc_topology_server_timestamp (mongoc_topology_t *topology, uint32_t id)
{
   mongoc_topology_scanner_node_t *node;
   int64_t timestamp = -1;

   mongoc_mutex_lock (&topology->mutex);

   node = mongoc_topology_scanner_get_node (topology->scanner, id);
   if (node) {
      timestamp = node->timestamp;
   }

   mongoc_mutex_unlock (&topology->mutex);

   return timestamp;
}

/*
 *--------------------------------------------------------------------------
 *
 * _mongoc_topology_get_type --
 *
 *      Return the topology's description's type.
 *
 *      NOTE: this method uses @topology's mutex.
 *
 * Returns:
 *      The topology description type.
 *
 *--------------------------------------------------------------------------
 */
mongoc_topology_description_type_t
_mongoc_topology_get_type (mongoc_topology_t *topology)
{
   mongoc_topology_description_type_t td_type;

   mongoc_mutex_lock (&topology->mutex);

   td_type = topology->description.type;

   mongoc_mutex_unlock (&topology->mutex);

   return td_type;
}

/*
 *--------------------------------------------------------------------------
 *
 * _mongoc_topology_run_background --
 *
 *       The background topology monitoring thread runs in this loop.
 *
 *       NOTE: this method uses @topology's mutex.
 *
 *--------------------------------------------------------------------------
 */
static void *
_mongoc_topology_run_background (void *data)
{
   mongoc_topology_t *topology;
   int64_t now;
   int64_t last_scan;
   int64_t timeout;
   int64_t force_timeout;
   int64_t heartbeat_msec;
   int r;

   BSON_ASSERT (data);

   last_scan = 0;
   topology = (mongoc_topology_t *) data;
   heartbeat_msec = topology->description.heartbeat_msec;

   /* we exit this loop when shutdown_requested, or on error */
   for (;;) {
      /* unlocked after starting a scan or after breaking out of the loop */
      mongoc_mutex_lock (&topology->mutex);

      /* we exit this loop on error, or when we should scan immediately */
      for (;;) {
         if (topology->shutdown_requested)
            goto DONE;

         now = bson_get_monotonic_time ();

         if (last_scan == 0) {
            /* set up the "last scan" as exactly long enough to force an
             * immediate scan on the first pass */
            last_scan = now - (heartbeat_msec * 1000);
         }

         timeout = heartbeat_msec - ((now - last_scan) / 1000);

         /* if someone's specifically asked for a scan, use a shorter interval
          */
         if (topology->scan_requested) {
            force_timeout = MONGOC_TOPOLOGY_MIN_HEARTBEAT_FREQUENCY_MS -
                            ((now - last_scan) / 1000);

            timeout = BSON_MIN (timeout, force_timeout);
         }

         /* if we can start scanning, do so immediately */
         if (timeout <= 0) {
            mongoc_topology_scanner_start (
               topology->scanner, topology->connect_timeout_msec, false);
            break;
         } else {
            /* otherwise wait until someone:
             *   o requests a scan
             *   o we time out
             *   o requests a shutdown
             */
            r = mongoc_cond_timedwait (
               &topology->cond_server, &topology->mutex, timeout);

#ifdef _WIN32
            if (!(r == 0 || r == WSAETIMEDOUT)) {
#else
            if (!(r == 0 || r == ETIMEDOUT)) {
#endif
               /* handle errors */
               goto DONE;
            }

            /* if we timed out, or were woken up, check if it's time to scan
             * again, or bail out */
         }
      }

      topology->scan_requested = false;

      /* scanning locks and unlocks the mutex itself until the scan is done */
      mongoc_mutex_unlock (&topology->mutex);
      mongoc_topology_scanner_work (topology->scanner);

      mongoc_mutex_lock (&topology->mutex);

      _mongoc_topology_scanner_finish (topology->scanner);
      /* "retired" nodes can be checked again in the next scan */
      mongoc_topology_scanner_reset (topology->scanner);

      topology->last_scan = bson_get_monotonic_time ();
      mongoc_mutex_unlock (&topology->mutex);

      last_scan = bson_get_monotonic_time ();
   }

DONE:
   mongoc_mutex_unlock (&topology->mutex);

   return NULL;
}

/*
 *--------------------------------------------------------------------------
 *
 * mongoc_topology_start_background_scanner
 *
 *       Start the topology background thread running. This should only be
 *       called once per pool. If clients are created separately (not
 *       through a pool) the SDAM logic will not be run in a background
 *       thread. Returns whether or not the scanner is running on termination
 *       of the function.
 *
 *       NOTE: this method uses @topology's mutex.
 *
 *--------------------------------------------------------------------------
 */

bool
_mongoc_topology_start_background_scanner (mongoc_topology_t *topology)
{
   int r;

   if (topology->single_threaded) {
      return false;
   }

   mongoc_mutex_lock (&topology->mutex);
   if (topology->scanner_state == MONGOC_TOPOLOGY_SCANNER_OFF) {
      topology->scanner_state = MONGOC_TOPOLOGY_SCANNER_BG_RUNNING;

      _mongoc_handshake_freeze ();
      _mongoc_topology_description_monitor_opening (&topology->description);

      r = mongoc_thread_create (
         &topology->thread, _mongoc_topology_run_background, topology);

      if (r != 0) {
         MONGOC_ERROR ("could not start topology scanner thread: %s",
                       strerror (r));
         abort ();
      }
   }

   mongoc_mutex_unlock (&topology->mutex);
   return true;
}

/*
 *--------------------------------------------------------------------------
 *
 * mongoc_topology_background_thread_stop --
 *
 *       Stop the topology background thread. Called by the owning pool at
 *       its destruction.
 *
 *       NOTE: this method uses @topology's mutex.
 *
 *--------------------------------------------------------------------------
 */

static void
_mongoc_topology_background_thread_stop (mongoc_topology_t *topology)
{
   bool join_thread = false;

   if (topology->single_threaded) {
      return;
   }

   mongoc_mutex_lock (&topology->mutex);
   if (topology->scanner_state == MONGOC_TOPOLOGY_SCANNER_BG_RUNNING) {
      /* if the background thread is running, request a shutdown and signal the
       * thread */
      topology->shutdown_requested = true;
      mongoc_cond_signal (&topology->cond_server);
      topology->scanner_state = MONGOC_TOPOLOGY_SCANNER_SHUTTING_DOWN;
      join_thread = true;
   } else if (topology->scanner_state ==
              MONGOC_TOPOLOGY_SCANNER_SHUTTING_DOWN) {
      /* if we're mid shutdown, wait until it shuts down */
      while (topology->scanner_state != MONGOC_TOPOLOGY_SCANNER_OFF) {
         mongoc_cond_wait (&topology->cond_client, &topology->mutex);
      }
   } else {
      /* nothing to do if it's already off */
   }

   mongoc_mutex_unlock (&topology->mutex);

   if (join_thread) {
      /* if we're joining the thread, wait for it to come back and broadcast
       * all listeners */
      mongoc_thread_join (topology->thread);
      mongoc_cond_broadcast (&topology->cond_client);
   }
}

bool
_mongoc_topology_set_appname (mongoc_topology_t *topology, const char *appname)
{
   bool ret = false;
   mongoc_mutex_lock (&topology->mutex);

   if (topology->scanner_state == MONGOC_TOPOLOGY_SCANNER_OFF) {
      ret = _mongoc_topology_scanner_set_appname (topology->scanner, appname);
   } else {
      MONGOC_ERROR ("Cannot set appname after handshake initiated");
   }
   mongoc_mutex_unlock (&topology->mutex);
   return ret;
}
