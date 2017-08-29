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

#include <bson.h>
#include <bson-string.h>

#include "mongoc-config.h"
#include "mongoc-error.h"
#include "mongoc-trace-private.h"
#include "mongoc-topology-scanner-private.h"
#include "mongoc-stream-socket.h"

#include "mongoc-handshake.h"
#include "mongoc-handshake-private.h"

#ifdef MONGOC_ENABLE_SSL
#include "mongoc-stream-tls.h"
#endif

#include "mongoc-counters-private.h"
#include "utlist.h"
#include "mongoc-topology-private.h"
#include "mongoc-host-list-private.h"

#undef MONGOC_LOG_DOMAIN
#define MONGOC_LOG_DOMAIN "topology_scanner"

/* forward declarations */
static void
mongoc_topology_scanner_ismaster_handler (
   mongoc_async_cmd_result_t async_status,
   const bson_t *ismaster_response,
   int64_t rtt_msec,
   void *data,
   bson_error_t *error);

static void
_mongoc_topology_scanner_monitor_heartbeat_started (
   const mongoc_topology_scanner_t *ts, const mongoc_host_list_t *host);

static void
_mongoc_topology_scanner_monitor_heartbeat_succeeded (
   const mongoc_topology_scanner_t *ts,
   const mongoc_host_list_t *host,
   const bson_t *reply);

static void
_mongoc_topology_scanner_monitor_heartbeat_failed (
   const mongoc_topology_scanner_t *ts,
   const mongoc_host_list_t *host,
   const bson_error_t *error);

static void
_add_ismaster (bson_t *cmd)
{
   BSON_APPEND_INT32 (cmd, "isMaster", 1);
}

static bool
_build_ismaster_with_handshake (mongoc_topology_scanner_t *ts)
{
   bson_t *doc = &ts->ismaster_cmd_with_handshake;
   bson_t subdoc;
   bson_iter_t iter;
   const char *key;
   int keylen;
   bool res;
   const bson_t *compressors;
   int count = 0;
   char buf[16];

   _add_ismaster (doc);

   BSON_APPEND_DOCUMENT_BEGIN (doc, HANDSHAKE_FIELD, &subdoc);
   res = _mongoc_handshake_build_doc_with_application (&subdoc, ts->appname);
   bson_append_document_end (doc, &subdoc);

   BSON_APPEND_ARRAY_BEGIN (doc, "compression", &subdoc);
   if (ts->uri) {
      compressors = mongoc_uri_get_compressors (ts->uri);

      if (bson_iter_init (&iter, compressors)) {
         while (bson_iter_next (&iter)) {
            keylen = bson_uint32_to_string (count++, &key, buf, sizeof buf);
            bson_append_utf8 (
               &subdoc, key, (int) keylen, bson_iter_key (&iter), -1);
         }
      }
   }
   bson_append_array_end (doc, &subdoc);

   /* Return whether the handshake doc fit the size limit */
   return res;
}

bson_t *
_mongoc_topology_scanner_get_ismaster (mongoc_topology_scanner_t *ts)
{
   /* If this is the first time using the node or if it's the first time
    * using it after a failure, build handshake doc */
   if (bson_empty (&ts->ismaster_cmd_with_handshake)) {
      ts->handshake_ok_to_send = _build_ismaster_with_handshake (ts);
      if (!ts->handshake_ok_to_send) {
         MONGOC_WARNING ("Handshake doc too big, not including in isMaster");
      }
   }

   /* If the doc turned out to be too big */
   if (!ts->handshake_ok_to_send) {
      return &ts->ismaster_cmd;
   }

   return &ts->ismaster_cmd_with_handshake;
}

static void
_begin_ismaster_cmd (mongoc_topology_scanner_t *ts,
                     mongoc_topology_scanner_node_t *node,
                     int64_t timeout_msec)
{
   const bson_t *command;

   if (node->last_used != -1 && node->last_failed == -1) {
      /* The node's been used before and not failed recently */
      command = &ts->ismaster_cmd;
   } else {
      command = _mongoc_topology_scanner_get_ismaster (ts);
   }


   node->cmd = mongoc_async_cmd_new (ts->async,
                                     node->stream,
                                     ts->setup,
                                     node->host.host,
                                     "admin",
                                     command,
                                     &mongoc_topology_scanner_ismaster_handler,
                                     node,
                                     timeout_msec);
}


mongoc_topology_scanner_t *
mongoc_topology_scanner_new (
   const mongoc_uri_t *uri,
   mongoc_topology_scanner_setup_err_cb_t setup_err_cb,
   mongoc_topology_scanner_cb_t cb,
   void *data)
{
   mongoc_topology_scanner_t *ts =
      (mongoc_topology_scanner_t *) bson_malloc0 (sizeof (*ts));

   ts->async = mongoc_async_new ();

   bson_init (&ts->ismaster_cmd);
   _add_ismaster (&ts->ismaster_cmd);
   bson_init (&ts->ismaster_cmd_with_handshake);

   ts->setup_err_cb = setup_err_cb;
   ts->cb = cb;
   ts->cb_data = data;
   ts->uri = uri;
   ts->appname = NULL;
   ts->handshake_ok_to_send = false;

   return ts;
}

#ifdef MONGOC_ENABLE_SSL
void
mongoc_topology_scanner_set_ssl_opts (mongoc_topology_scanner_t *ts,
                                      mongoc_ssl_opt_t *opts)
{
   ts->ssl_opts = opts;
   ts->setup = mongoc_async_cmd_tls_setup;
}
#endif

void
mongoc_topology_scanner_set_stream_initiator (mongoc_topology_scanner_t *ts,
                                              mongoc_stream_initiator_t si,
                                              void *ctx)
{
   ts->initiator = si;
   ts->initiator_context = ctx;
   ts->setup = NULL;
}

void
mongoc_topology_scanner_destroy (mongoc_topology_scanner_t *ts)
{
   mongoc_topology_scanner_node_t *ele, *tmp;

   DL_FOREACH_SAFE (ts->nodes, ele, tmp)
   {
      mongoc_topology_scanner_node_destroy (ele, false);
   }

   mongoc_async_destroy (ts->async);
   bson_destroy (&ts->ismaster_cmd);
   bson_destroy (&ts->ismaster_cmd_with_handshake);

   /* This field can be set by a mongoc_client */
   bson_free ((char *) ts->appname);

   bson_free (ts);
}

void
mongoc_topology_scanner_add (mongoc_topology_scanner_t *ts,
                             const mongoc_host_list_t *host,
                             uint32_t id)
{
   mongoc_topology_scanner_node_t *node;

   node = (mongoc_topology_scanner_node_t *) bson_malloc0 (sizeof (*node));

   memcpy (&node->host, host, sizeof (*host));

   node->id = id;
   node->ts = ts;
   node->last_failed = -1;
   node->last_used = -1;

   DL_APPEND (ts->nodes, node);
}

void
mongoc_topology_scanner_scan (mongoc_topology_scanner_t *ts,
                              uint32_t id,
                              int64_t timeout_msec)
{
   mongoc_topology_scanner_node_t *node;

   BSON_ASSERT (timeout_msec < INT32_MAX);

   node = mongoc_topology_scanner_get_node (ts, id);

   /* begin non-blocking connection, don't wait for success */
   if (node && mongoc_topology_scanner_node_setup (node, &node->last_error)) {
      _begin_ismaster_cmd (ts, node, timeout_msec);
   }

   /* if setup fails the node stays in the scanner. destroyed after the scan. */
}

void
mongoc_topology_scanner_node_retire (mongoc_topology_scanner_node_t *node)
{
   if (node->cmd) {
      node->cmd->state = MONGOC_ASYNC_CMD_CANCELED_STATE;
   }

   node->retired = true;
}

void
mongoc_topology_scanner_node_disconnect (mongoc_topology_scanner_node_t *node,
                                         bool failed)
{
   if (node->dns_results) {
      freeaddrinfo (node->dns_results);
      node->dns_results = NULL;
      node->current_dns_result = NULL;
   }

   if (node->cmd) {
      mongoc_async_cmd_destroy (node->cmd);
      node->cmd = NULL;
   }

   if (node->stream) {
      if (failed) {
         mongoc_stream_failed (node->stream);
      } else {
         mongoc_stream_destroy (node->stream);
      }

      node->stream = NULL;
   }
}

void
mongoc_topology_scanner_node_destroy (mongoc_topology_scanner_node_t *node,
                                      bool failed)
{
   DL_DELETE (node->ts->nodes, node);
   mongoc_topology_scanner_node_disconnect (node, failed);
   bson_free (node);
}

/*
 *--------------------------------------------------------------------------
 *
 * mongoc_topology_scanner_get_node --
 *
 *      Return the scanner node with the given id.
 *
 *--------------------------------------------------------------------------
 */
mongoc_topology_scanner_node_t *
mongoc_topology_scanner_get_node (mongoc_topology_scanner_t *ts, uint32_t id)
{
   mongoc_topology_scanner_node_t *ele, *tmp;

   DL_FOREACH_SAFE (ts->nodes, ele, tmp)
   {
      if (ele->id == id) {
         return ele;
      }

      if (ele->id > id) {
         break;
      }
   }

   return NULL;
}

/*
 *--------------------------------------------------------------------------
 *
 * mongoc_topology_scanner_has_node_for_host --
 *
 *      Whether the scanner has a node for the given host and port.
 *
 *--------------------------------------------------------------------------
 */
bool
mongoc_topology_scanner_has_node_for_host (mongoc_topology_scanner_t *ts,
                                           mongoc_host_list_t *host)
{
   mongoc_topology_scanner_node_t *ele, *tmp;

   DL_FOREACH_SAFE (ts->nodes, ele, tmp)
   {
      if (_mongoc_host_list_equal (&ele->host, host)) {
         return true;
      }
   }

   return false;
}

/*
 *-----------------------------------------------------------------------
 *
 * This is the callback passed to async_cmd when we're running
 * ismasters from within the topology monitor.
 *
 *-----------------------------------------------------------------------
 */

static void
mongoc_topology_scanner_ismaster_handler (
   mongoc_async_cmd_result_t async_status,
   const bson_t *ismaster_response,
   int64_t rtt_msec,
   void *data,
   bson_error_t *error)
{
   mongoc_topology_scanner_node_t *node;
   mongoc_topology_scanner_t *ts;
   int64_t now;
   const char *message;

   BSON_ASSERT (data);

   node = (mongoc_topology_scanner_node_t *) data;
   ts = node->ts;
   node->cmd = NULL;

   if (node->retired) {
      return;
   }

   now = bson_get_monotonic_time ();

   /* if no ismaster response, async cmd had an error or timed out */
   if (!ismaster_response || async_status == MONGOC_ASYNC_CMD_ERROR ||
       async_status == MONGOC_ASYNC_CMD_TIMEOUT) {
      mongoc_stream_failed (node->stream);
      node->stream = NULL;
      node->last_failed = now;
      if (error->code) {
         message = error->message;
      } else {
         if (async_status == MONGOC_ASYNC_CMD_TIMEOUT) {
            message = "connection timeout";
         } else {
            message = "connection error";
         }
      }

      bson_set_error (&node->last_error,
                      MONGOC_ERROR_CLIENT,
                      MONGOC_ERROR_STREAM_CONNECT,
                      "%s calling ismaster on \'%s\'",
                      message,
                      node->host.host_and_port);

      _mongoc_topology_scanner_monitor_heartbeat_failed (
         ts, &node->host, &node->last_error);
   } else {
      node->last_failed = -1;
      _mongoc_topology_scanner_monitor_heartbeat_succeeded (
         ts, &node->host, ismaster_response);
   }

   node->last_used = now;
   ts->cb (node->id, ismaster_response, rtt_msec, ts->cb_data, error);
}


/*
 *--------------------------------------------------------------------------
 *
 * mongoc_topology_scanner_node_connect_tcp --
 *
 *      Create a socket stream for this node, begin a non-blocking
 *      connect and return.
 *
 * Returns:
 *      A stream. On failure, return NULL and fill out the error.
 *
 *--------------------------------------------------------------------------
 */

static mongoc_stream_t *
mongoc_topology_scanner_node_connect_tcp (mongoc_topology_scanner_node_t *node,
                                          bson_error_t *error)
{
   mongoc_socket_t *sock = NULL;
   struct addrinfo hints;
   struct addrinfo *rp;
   char portstr[8];
   mongoc_host_list_t *host;
   int s;

   ENTRY;

   host = &node->host;

   if (!node->dns_results) {
      bson_snprintf (portstr, sizeof portstr, "%hu", host->port);

      memset (&hints, 0, sizeof hints);
      hints.ai_family = host->family;
      hints.ai_socktype = SOCK_STREAM;
      hints.ai_flags = 0;
      hints.ai_protocol = 0;

      s = getaddrinfo (host->host, portstr, &hints, &node->dns_results);

      if (s != 0) {
         mongoc_counter_dns_failure_inc ();
         bson_set_error (error,
                         MONGOC_ERROR_STREAM,
                         MONGOC_ERROR_STREAM_NAME_RESOLUTION,
                         "Failed to resolve '%s'",
                         host->host);
         RETURN (NULL);
      }

      node->current_dns_result = node->dns_results;

      mongoc_counter_dns_success_inc ();
   }

   for (; node->current_dns_result;
        node->current_dns_result = node->current_dns_result->ai_next) {
      rp = node->current_dns_result;

      /*
       * Create a new non-blocking socket.
       */
      if (!(sock = mongoc_socket_new (
               rp->ai_family, rp->ai_socktype, rp->ai_protocol))) {
         continue;
      }

      mongoc_socket_connect (
         sock, rp->ai_addr, (mongoc_socklen_t) rp->ai_addrlen, 0);

      break;
   }

   if (!sock) {
      bson_set_error (error,
                      MONGOC_ERROR_STREAM,
                      MONGOC_ERROR_STREAM_CONNECT,
                      "Failed to connect to target host: '%s'",
                      host->host_and_port);
      freeaddrinfo (node->dns_results);
      node->dns_results = NULL;
      node->current_dns_result = NULL;
      RETURN (NULL);
   }

   return mongoc_stream_socket_new (sock);
}

static mongoc_stream_t *
mongoc_topology_scanner_node_connect_unix (mongoc_topology_scanner_node_t *node,
                                           bson_error_t *error)
{
#ifdef _WIN32
   ENTRY;
   bson_set_error (error,
                   MONGOC_ERROR_STREAM,
                   MONGOC_ERROR_STREAM_CONNECT,
                   "UNIX domain sockets not supported on win32.");
   RETURN (NULL);
#else
   struct sockaddr_un saddr;
   mongoc_socket_t *sock;
   mongoc_stream_t *ret = NULL;
   mongoc_host_list_t *host;

   ENTRY;

   host = &node->host;

   memset (&saddr, 0, sizeof saddr);
   saddr.sun_family = AF_UNIX;
   bson_snprintf (saddr.sun_path, sizeof saddr.sun_path - 1, "%s", host->host);

   sock = mongoc_socket_new (AF_UNIX, SOCK_STREAM, 0);

   if (sock == NULL) {
      bson_set_error (error,
                      MONGOC_ERROR_STREAM,
                      MONGOC_ERROR_STREAM_SOCKET,
                      "Failed to create socket.");
      RETURN (NULL);
   }

   if (-1 == mongoc_socket_connect (
                sock, (struct sockaddr *) &saddr, sizeof saddr, -1)) {
      char buf[128];
      char *errstr;

      errstr = bson_strerror_r (mongoc_socket_errno (sock), buf, sizeof (buf));

      bson_set_error (error,
                      MONGOC_ERROR_STREAM,
                      MONGOC_ERROR_STREAM_CONNECT,
                      "Failed to connect to UNIX domain socket: %s",
                      errstr);
      mongoc_socket_destroy (sock);
      RETURN (NULL);
   }

   ret = mongoc_stream_socket_new (sock);

   RETURN (ret);
#endif
}


/*
 *--------------------------------------------------------------------------
 *
 * mongoc_topology_scanner_node_setup --
 *
 *      Create a stream and begin a non-blocking connect.
 *
 * Returns:
 *      true on success, or false and error is set.
 *
 *--------------------------------------------------------------------------
 */

bool
mongoc_topology_scanner_node_setup (mongoc_topology_scanner_node_t *node,
                                    bson_error_t *error)
{
   mongoc_stream_t *sock_stream;

   _mongoc_topology_scanner_monitor_heartbeat_started (node->ts, &node->host);

   if (node->stream) {
      return true;
   }

   BSON_ASSERT (!node->retired);

   if (node->ts->initiator) {
      sock_stream = node->ts->initiator (
         node->ts->uri, &node->host, node->ts->initiator_context, error);
   } else {
      if (node->host.family == AF_UNIX) {
         sock_stream = mongoc_topology_scanner_node_connect_unix (node, error);
      } else {
         sock_stream = mongoc_topology_scanner_node_connect_tcp (node, error);
      }

#ifdef MONGOC_ENABLE_SSL
      if (sock_stream && node->ts->ssl_opts) {
         mongoc_stream_t *original = sock_stream;

         sock_stream = mongoc_stream_tls_new_with_hostname (
            sock_stream, node->host.host, node->ts->ssl_opts, 1);
         if (!sock_stream) {
            mongoc_stream_destroy (original);
         }
      }
#endif
   }

   if (!sock_stream) {
      _mongoc_topology_scanner_monitor_heartbeat_failed (
         node->ts, &node->host, error);

      node->ts->setup_err_cb (node->id, node->ts->cb_data, error);
      return false;
   }

   node->stream = sock_stream;
   node->has_auth = false;
   node->timestamp = bson_get_monotonic_time ();

   return true;
}

/*
 *--------------------------------------------------------------------------
 *
 * mongoc_topology_scanner_start --
 *
 *      Initializes the scanner and begins a full topology check. This
 *      should be called once before calling mongoc_topology_scanner_work()
 *      repeatedly to complete the scan.
 *
 *      If "obey_cooldown" is true, this is a single-threaded blocking scan
 *      that must obey the Server Discovery And Monitoring Spec's cooldownMS:
 *
 *      "After a single-threaded client gets a network error trying to check
 *      a server, the client skips re-checking the server until cooldownMS has
 *      passed.
 *
 *      "This avoids spending connectTimeoutMS on each unavailable server
 *      during each scan.
 *
 *      "This value MUST be 5000 ms, and it MUST NOT be configurable."
 *
 *--------------------------------------------------------------------------
 */

void
mongoc_topology_scanner_start (mongoc_topology_scanner_t *ts,
                               int64_t timeout_msec,
                               bool obey_cooldown)
{
   mongoc_topology_scanner_node_t *node, *tmp;
   int64_t cooldown = INT64_MAX;
   BSON_ASSERT (ts);

   if (ts->in_progress) {
      return;
   }


   if (obey_cooldown) {
      /* when current cooldown period began */
      cooldown =
         bson_get_monotonic_time () - 1000 * MONGOC_TOPOLOGY_COOLDOWN_MS;
   }

   DL_FOREACH_SAFE (ts->nodes, node, tmp)
   {
      /* check node if it last failed before current cooldown period began */
      if (node->last_failed < cooldown) {
         if (mongoc_topology_scanner_node_setup (node, &node->last_error)) {
            BSON_ASSERT (!node->cmd);
            _begin_ismaster_cmd (ts, node, timeout_msec);
         }
      }
   }
}

/*
 *--------------------------------------------------------------------------
 *
 * mongoc_topology_scanner_finish_scan --
 *
 *      Summarizes all scanner node errors into one error message.
 *
 *--------------------------------------------------------------------------
 */

void
_mongoc_topology_scanner_finish (mongoc_topology_scanner_t *ts)
{
   mongoc_topology_scanner_node_t *node, *tmp;
   bson_error_t *error = &ts->error;
   bson_string_t *msg;

   memset (&ts->error, 0, sizeof (bson_error_t));

   msg = bson_string_new (NULL);

   DL_FOREACH_SAFE (ts->nodes, node, tmp)
   {
      if (node->last_error.code) {
         if (msg->len) {
            bson_string_append_c (msg, ' ');
         }

         bson_string_append_printf (msg, "[%s]", node->last_error.message);

         /* last error domain and code win */
         error->domain = node->last_error.domain;
         error->code = node->last_error.code;
      }
   }

   bson_strncpy ((char *) &error->message, msg->str, sizeof (error->message));
   bson_string_free (msg, true);
}

/*
 *--------------------------------------------------------------------------
 *
 * mongoc_topology_scanner_work --
 *
 *      Crank the knob on the topology scanner state machine. This should
 *      be called only after mongoc_topology_scanner_start() has been used
 *      to begin the scan.
 *
 *--------------------------------------------------------------------------
 */

void
mongoc_topology_scanner_work (mongoc_topology_scanner_t *ts)
{
   mongoc_async_run (ts->async);
}

/*
 *--------------------------------------------------------------------------
 *
 * mongoc_topology_scanner_get_error --
 *
 *      Copy the scanner's current error; which may no-error (code 0).
 *
 *--------------------------------------------------------------------------
 */

void
mongoc_topology_scanner_get_error (mongoc_topology_scanner_t *ts,
                                   bson_error_t *error)
{
   BSON_ASSERT (ts);
   BSON_ASSERT (error);

   memcpy (error, &ts->error, sizeof (bson_error_t));
}

/*
 *--------------------------------------------------------------------------
 *
 * mongoc_topology_scanner_reset --
 *
 *      Reset "retired" nodes that failed or were removed in the previous
 *      scan.
 *
 *--------------------------------------------------------------------------
 */

void
mongoc_topology_scanner_reset (mongoc_topology_scanner_t *ts)
{
   mongoc_topology_scanner_node_t *node, *tmp;

   DL_FOREACH_SAFE (ts->nodes, node, tmp)
   {
      if (node->retired) {
         mongoc_topology_scanner_node_destroy (node, true);
      }
   }
}

/*
 * Set a field in the topology scanner.
 */
bool
_mongoc_topology_scanner_set_appname (mongoc_topology_scanner_t *ts,
                                      const char *appname)
{
   if (!_mongoc_handshake_appname_is_valid (appname)) {
      MONGOC_ERROR ("Cannot set appname: %s is invalid", appname);
      return false;
   }

   if (ts->appname != NULL) {
      MONGOC_ERROR ("Cannot set appname more than once");
      return false;
   }

   ts->appname = bson_strdup (appname);
   return true;
}

/* SDAM Monitoring Spec: send HeartbeatStartedEvent */
static void
_mongoc_topology_scanner_monitor_heartbeat_started (
   const mongoc_topology_scanner_t *ts, const mongoc_host_list_t *host)
{
   if (ts->apm_callbacks.server_heartbeat_started) {
      mongoc_apm_server_heartbeat_started_t event;
      event.host = host;
      event.context = ts->apm_context;
      ts->apm_callbacks.server_heartbeat_started (&event);
   }
}

/* SDAM Monitoring Spec: send HeartbeatSucceededEvent */
static void
_mongoc_topology_scanner_monitor_heartbeat_succeeded (
   const mongoc_topology_scanner_t *ts,
   const mongoc_host_list_t *host,
   const bson_t *reply)
{
   if (ts->apm_callbacks.server_heartbeat_succeeded) {
      mongoc_apm_server_heartbeat_succeeded_t event;
      event.host = host;
      event.context = ts->apm_context;
      event.reply = reply;
      ts->apm_callbacks.server_heartbeat_succeeded (&event);
   }
}

/* SDAM Monitoring Spec: send HeartbeatFailedEvent */
static void
_mongoc_topology_scanner_monitor_heartbeat_failed (
   const mongoc_topology_scanner_t *ts,
   const mongoc_host_list_t *host,
   const bson_error_t *error)
{
   if (ts->apm_callbacks.server_heartbeat_failed) {
      mongoc_apm_server_heartbeat_failed_t event;
      event.host = host;
      event.context = ts->apm_context;
      event.error = error;
      ts->apm_callbacks.server_heartbeat_failed (&event);
   }
}
