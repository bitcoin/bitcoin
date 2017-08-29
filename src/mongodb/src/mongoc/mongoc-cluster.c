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


#include "mongoc-config.h"

#include <string.h>

#include "mongoc-cluster-private.h"
#include "mongoc-client-private.h"
#include "mongoc-counters-private.h"
#include "mongoc-config.h"
#include "mongoc-error.h"
#include "mongoc-host-list-private.h"
#include "mongoc-log.h"
#ifdef MONGOC_ENABLE_SASL
#include "mongoc-cluster-sasl-private.h"
#endif
#ifdef MONGOC_ENABLE_SSL
#include "mongoc-ssl.h"
#include "mongoc-ssl-private.h"
#include "mongoc-stream-tls.h"
#endif
#include "mongoc-b64-private.h"
#include "mongoc-scram-private.h"
#include "mongoc-set-private.h"
#include "mongoc-socket.h"
#include "mongoc-stream-private.h"
#include "mongoc-stream-socket.h"
#include "mongoc-stream-tls.h"
#include "mongoc-thread-private.h"
#include "mongoc-topology-private.h"
#include "mongoc-trace-private.h"
#include "mongoc-util-private.h"
#include "mongoc-write-concern-private.h"
#include "mongoc-uri-private.h"
#include "mongoc-rpc-private.h"
#include "mongoc-compression-private.h"
#include "mongoc-cmd-private.h"

#undef MONGOC_LOG_DOMAIN
#define MONGOC_LOG_DOMAIN "cluster"


#define CHECK_CLOSED_DURATION_MSEC 1000

#define DB_AND_CMD_FROM_COLLECTION(outstr, name)              \
   do {                                                       \
      const char *dot = strchr (name, '.');                   \
      if (!dot || ((dot - name) > (sizeof outstr - 6))) {     \
         bson_snprintf (outstr, sizeof outstr, "admin.$cmd"); \
      } else {                                                \
         memcpy (outstr, name, dot - name);                   \
         memcpy (outstr + (dot - name), ".$cmd", 6);          \
      }                                                       \
   } while (0)

#define IS_NOT_COMMAND(name) (!!strcasecmp (command_name, name))

static mongoc_server_stream_t *
mongoc_cluster_fetch_stream_single (mongoc_cluster_t *cluster,
                                    uint32_t server_id,
                                    bool reconnect_ok,
                                    bson_error_t *error);

static mongoc_server_stream_t *
mongoc_cluster_fetch_stream_pooled (mongoc_cluster_t *cluster,
                                    uint32_t server_id,
                                    bool reconnect_ok,
                                    bson_error_t *error);

static void
_bson_error_message_printf (bson_error_t *error, const char *format, ...)
   BSON_GNUC_PRINTF (2, 3);


/*
 *--------------------------------------------------------------------------
 *
 * _mongoc_cluster_inc_egress_rpc --
 *
 *       Helper to increment the counter for a particular RPC based on
 *       it's opcode.
 *
 * Returns:
 *       None.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

static void
_mongoc_cluster_inc_egress_rpc (const mongoc_rpc_t *rpc)
{
   mongoc_counter_op_egress_total_inc ();

   switch (rpc->header.opcode) {
   case MONGOC_OPCODE_DELETE:
      mongoc_counter_op_egress_delete_inc ();
      break;
   case MONGOC_OPCODE_UPDATE:
      mongoc_counter_op_egress_update_inc ();
      break;
   case MONGOC_OPCODE_INSERT:
      mongoc_counter_op_egress_insert_inc ();
      break;
   case MONGOC_OPCODE_KILL_CURSORS:
      mongoc_counter_op_egress_killcursors_inc ();
      break;
   case MONGOC_OPCODE_GET_MORE:
      mongoc_counter_op_egress_getmore_inc ();
      break;
   case MONGOC_OPCODE_REPLY:
      mongoc_counter_op_egress_reply_inc ();
      break;
   case MONGOC_OPCODE_MSG:
      mongoc_counter_op_egress_msg_inc ();
      break;
   case MONGOC_OPCODE_QUERY:
      mongoc_counter_op_egress_query_inc ();
      break;
   case MONGOC_OPCODE_COMPRESSED:
      mongoc_counter_op_egress_compressed_inc ();
      break;
   default:
      BSON_ASSERT (false);
      break;
   }
}

/*
 *--------------------------------------------------------------------------
 *
 * _mongoc_cluster_inc_ingress_rpc --
 *
 *       Helper to increment the counter for a particular RPC based on
 *       it's opcode.
 *
 * Returns:
 *       None.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

static void
_mongoc_cluster_inc_ingress_rpc (const mongoc_rpc_t *rpc)
{
   mongoc_counter_op_ingress_total_inc ();

   switch (rpc->header.opcode) {
   case MONGOC_OPCODE_DELETE:
      mongoc_counter_op_ingress_delete_inc ();
      break;
   case MONGOC_OPCODE_UPDATE:
      mongoc_counter_op_ingress_update_inc ();
      break;
   case MONGOC_OPCODE_INSERT:
      mongoc_counter_op_ingress_insert_inc ();
      break;
   case MONGOC_OPCODE_KILL_CURSORS:
      mongoc_counter_op_ingress_killcursors_inc ();
      break;
   case MONGOC_OPCODE_GET_MORE:
      mongoc_counter_op_ingress_getmore_inc ();
      break;
   case MONGOC_OPCODE_REPLY:
      mongoc_counter_op_ingress_reply_inc ();
      break;
   case MONGOC_OPCODE_MSG:
      mongoc_counter_op_ingress_msg_inc ();
      break;
   case MONGOC_OPCODE_QUERY:
      mongoc_counter_op_ingress_query_inc ();
      break;
   case MONGOC_OPCODE_COMPRESSED:
      mongoc_counter_op_ingress_compressed_inc ();
      break;
   default:
      BSON_ASSERT (false);
      break;
   }
}


size_t
_mongoc_cluster_buffer_iovec (mongoc_iovec_t *iov,
                              size_t iovcnt,
                              int skip,
                              char *buffer)
{
   int n;
   size_t buffer_offset = 0;
   int total_iov_len = 0;
   int difference = 0;

   for (n = 0; n < iovcnt; n++) {
      total_iov_len += iov[n].iov_len;

      if (total_iov_len <= skip) {
         continue;
      }

      /* If this iovec starts before the skip, and takes the total count
       * beyond the skip, we need to figure out the portion of the iovec
       * we should skip passed */
      if (total_iov_len - iov[n].iov_len < skip) {
         difference = skip - (total_iov_len - iov[n].iov_len);
      } else {
         difference = 0;
      }

      memcpy (buffer + buffer_offset,
              iov[n].iov_base + difference,
              iov[n].iov_len - difference);
      buffer_offset += iov[n].iov_len - difference;
   }

   return buffer_offset;
}

/* Allows caller to safely overwrite error->message with a formatted string,
 * even if the formatted string includes original error->message. */
static void
_bson_error_message_printf (bson_error_t *error, const char *format, ...)
{
   va_list args;
   char error_message[sizeof error->message];

   if (error) {
      va_start (args, format);
      bson_vsnprintf (error_message, sizeof error->message, format, args);
      va_end (args);

      bson_strncpy (error->message, error_message, sizeof error->message);
   }
}

#define RUN_CMD_ERR(_domain, _code, _msg)                          \
   do {                                                            \
      bson_set_error (error, _domain, _code, _msg);                \
      _bson_error_message_printf (                                 \
         error,                                                    \
         "Failed to send \"%s\" command with database \"%s\": %s", \
         command_name,                                             \
         cmd->db_name,                                             \
         error->message);                                          \
   } while (0)

/*
 *--------------------------------------------------------------------------
 *
 * mongoc_cluster_run_command_internal --
 *
 *       Internal function to run a command on a given stream.
 *       @error and @reply are optional out-pointers.
 *
 * Returns:
 *       true if successful; otherwise false and @error is set.
 *
 * Side effects:
 *       @reply is set and should ALWAYS be released with bson_destroy().
 *       On failure, @error is filled out. If this was a network error
 *       and server_id is nonzero, the cluster disconnects from the server.
 *
 *--------------------------------------------------------------------------
 */

static bool
mongoc_cluster_run_command_internal (mongoc_cluster_t *cluster,
                                     mongoc_cmd_t *cmd,
                                     mongoc_stream_t *stream,
                                     int32_t compressor_id,
                                     bool monitored,
                                     const mongoc_host_list_t *host,
                                     bson_t *reply,
                                     bson_error_t *error)
{
   int64_t started;
   const char *command_name;
   mongoc_apm_callbacks_t *callbacks;
   const size_t reply_header_size = sizeof (mongoc_rpc_reply_header_t);
   uint8_t reply_header_buf[sizeof (mongoc_rpc_reply_header_t)];
   uint8_t *reply_buf;     /* reply body */
   mongoc_rpc_t rpc;       /* sent to server */
   bson_error_t err_local; /* in case the passed-in "error" is NULL */
   bson_t reply_local;
   bson_t *reply_ptr;
   char cmd_ns[MONGOC_NAMESPACE_MAX];
   uint32_t request_id;
   int32_t msg_len;
   size_t doc_len;
   mongoc_apm_command_started_t started_event;
   mongoc_apm_command_succeeded_t succeeded_event;
   mongoc_apm_command_failed_t failed_event;
   bool ret = false;
#ifdef MONGOC_ENABLE_COMPRESSION
   char *output = NULL;
#endif

   ENTRY;

   BSON_ASSERT (cluster);
   BSON_ASSERT (cmd);
   BSON_ASSERT (stream);

   started = bson_get_monotonic_time ();

   /*
    * setup
    */
   reply_ptr = reply ? reply : &reply_local;
   bson_init (reply_ptr);
   callbacks = &cluster->client->apm_callbacks;

   if (!error) {
      error = &err_local;
   }

   error->code = 0;

   /*
    * prepare the request
    */

   command_name = _mongoc_get_command_name (cmd->command);
   if (!command_name) {
      bson_set_error (error,
                      MONGOC_ERROR_COMMAND,
                      MONGOC_ERROR_COMMAND_INVALID_ARG,
                      "Empty command document");

      /* haven't fired command-started event, so don't fire command-failed */
      monitored = false;
      GOTO (done);
   }

   _mongoc_array_clear (&cluster->iov);

   bson_snprintf (cmd_ns, sizeof cmd_ns, "%s.$cmd", cmd->db_name);
   request_id = ++cluster->request_id;
   _mongoc_rpc_prep_command (&rpc, cmd_ns, cmd);
   rpc.header.request_id = request_id;

   _mongoc_cluster_inc_egress_rpc (&rpc);
   _mongoc_rpc_gather (&rpc, &cluster->iov);
   _mongoc_rpc_swab_to_le (&rpc);

#ifdef MONGOC_ENABLE_COMPRESSION
   if (compressor_id && IS_NOT_COMMAND ("ismaster") &&
       IS_NOT_COMMAND ("saslstart") && IS_NOT_COMMAND ("saslcontinue") &&
       IS_NOT_COMMAND ("getnonce") && IS_NOT_COMMAND ("authenticate") &&
       IS_NOT_COMMAND ("createuser") && IS_NOT_COMMAND ("updateuser") &&
       IS_NOT_COMMAND ("copydbsaslstart") &&
       IS_NOT_COMMAND ("copydbgetnonce") && IS_NOT_COMMAND ("copydb")) {
      output = _mongoc_rpc_compress (cluster, compressor_id, &rpc, error);
      if (output == NULL) {
         monitored = false;
         GOTO (done);
      }
   }
#endif

   if (monitored && callbacks->started) {
      mongoc_apm_command_started_init (&started_event,
                                       cmd->command,
                                       cmd->db_name,
                                       command_name,
                                       request_id,
                                       cmd->operation_id,
                                       host,
                                       cmd->server_id,
                                       cluster->client->apm_context);

      callbacks->started (&started_event);
      mongoc_apm_command_started_cleanup (&started_event);
   }

   if (cluster->client->in_exhaust) {
      bson_set_error (error,
                      MONGOC_ERROR_CLIENT,
                      MONGOC_ERROR_CLIENT_IN_EXHAUST,
                      "A cursor derived from this client is in exhaust.");
      GOTO (done);
   }

   /*
    * send and receive
    */
   if (!_mongoc_stream_writev_full (stream,
                                    cluster->iov.data,
                                    cluster->iov.len,
                                    cluster->sockettimeoutms,
                                    error)) {
      mongoc_cluster_disconnect_node (cluster, cmd->server_id);

      /* add info about the command to writev_full's error message */
      _bson_error_message_printf (
         error,
         "Failed to send \"%s\" command with database \"%s\": %s",
         command_name,
         cmd->db_name,
         error->message);

      GOTO (done);
   }

   if (reply_header_size != mongoc_stream_read (stream,
                                                &reply_header_buf,
                                                reply_header_size,
                                                reply_header_size,
                                                cluster->sockettimeoutms)) {
      mongoc_cluster_disconnect_node (cluster, cmd->server_id);
      RUN_CMD_ERR (MONGOC_ERROR_STREAM,
                   MONGOC_ERROR_STREAM_SOCKET,
                   "socket error or timeout");

      GOTO (done);
   }

   memcpy (&msg_len, reply_header_buf, 4);
   msg_len = BSON_UINT32_FROM_LE (msg_len);
   if ((msg_len < reply_header_size) ||
       (msg_len > MONGOC_DEFAULT_MAX_MSG_SIZE)) {
      GOTO (done);
   }

   if (!_mongoc_rpc_scatter_reply_header_only (
          &rpc, reply_header_buf, reply_header_size)) {
      GOTO (done);
   }
   doc_len = (size_t) msg_len - reply_header_size;

   if (BSON_UINT32_FROM_LE (rpc.header.opcode) == MONGOC_OPCODE_COMPRESSED) {
      bson_t tmp = BSON_INITIALIZER;
      uint8_t *buf = NULL;
      size_t len = BSON_UINT32_FROM_LE (rpc.compressed.uncompressed_size) +
                   sizeof (mongoc_rpc_header_t);

      reply_buf = bson_malloc0 (msg_len);
      memcpy (reply_buf, reply_header_buf, reply_header_size);

      if (doc_len != mongoc_stream_read (stream,
                                         reply_buf + reply_header_size,
                                         doc_len,
                                         doc_len,
                                         cluster->sockettimeoutms)) {
         RUN_CMD_ERR (MONGOC_ERROR_STREAM,
                      MONGOC_ERROR_STREAM_SOCKET,
                      "socket error or timeout");
         GOTO (done);
      }
      if (!_mongoc_rpc_scatter (&rpc, reply_buf, msg_len)) {
         GOTO (done);
      }

      buf = bson_malloc0 (len);
      if (!_mongoc_rpc_decompress (&rpc, buf, len)) {
         RUN_CMD_ERR (MONGOC_ERROR_PROTOCOL,
                      MONGOC_ERROR_PROTOCOL_INVALID_REPLY,
                      "Could not decompress server reply");
         bson_free (reply_buf);
         bson_free (buf);
         GOTO (done);
      }

      _mongoc_rpc_swab_from_le (&rpc);
      _mongoc_cluster_inc_ingress_rpc (&rpc);

      _mongoc_rpc_get_first_document (&rpc, &tmp);
      bson_copy_to (&tmp, reply_ptr);
      bson_free (reply_buf);
      bson_free (buf);
   } else if (BSON_UINT32_FROM_LE (rpc.header.opcode) == MONGOC_OPCODE_REPLY &&
              BSON_UINT32_FROM_LE (rpc.reply_header.n_returned) == 1) {
      reply_buf = bson_reserve_buffer (reply_ptr, (uint32_t) doc_len);
      BSON_ASSERT (reply_buf);

      if (doc_len != mongoc_stream_read (stream,
                                         (void *) reply_buf,
                                         doc_len,
                                         doc_len,
                                         cluster->sockettimeoutms)) {
         RUN_CMD_ERR (MONGOC_ERROR_STREAM,
                      MONGOC_ERROR_STREAM_SOCKET,
                      "socket error or timeout");
         GOTO (done);
      }
      _mongoc_rpc_swab_from_le (&rpc);
   } else {
      GOTO (done);
   }
   _mongoc_cluster_inc_ingress_rpc (&rpc);

   if (!_mongoc_cmd_check_ok (
          reply_ptr, cluster->client->error_api_version, error)) {
      GOTO (done);
   }

   ret = true;
   if (monitored && callbacks->succeeded) {
      mongoc_apm_command_succeeded_init (&succeeded_event,
                                         bson_get_monotonic_time () - started,
                                         reply_ptr,
                                         command_name,
                                         request_id,
                                         cmd->operation_id,
                                         host,
                                         cmd->server_id,
                                         cluster->client->apm_context);

      callbacks->succeeded (&succeeded_event);
      mongoc_apm_command_succeeded_cleanup (&succeeded_event);
   }

done:

   if (!ret && error->code == 0) {
      /* generic error */
      RUN_CMD_ERR (MONGOC_ERROR_PROTOCOL,
                   MONGOC_ERROR_PROTOCOL_INVALID_REPLY,
                   "Invalid reply from server.");
   }

   if (!ret && monitored && callbacks->failed) {
      mongoc_apm_command_failed_init (&failed_event,
                                      bson_get_monotonic_time () - started,
                                      command_name,
                                      error,
                                      request_id,
                                      cmd->operation_id,
                                      host,
                                      cmd->server_id,
                                      cluster->client->apm_context);

      callbacks->failed (&failed_event);
      mongoc_apm_command_failed_cleanup (&failed_event);
   }

   if (reply_ptr == &reply_local) {
      bson_destroy (reply_ptr);
   }
#ifdef MONGOC_ENABLE_COMPRESSION
   bson_free (output);
#endif

   RETURN (ret);
}

/*
 *--------------------------------------------------------------------------
 *
 * mongoc_cluster_run_command_monitored --
 *
 *       Internal function to run a command on a given stream.
 *       @error and @reply are optional out-pointers.
 *
 * Returns:
 *       true if successful; otherwise false and @error is set.
 *
 * Side effects:
 *       If the client's APM callbacks are set, they are executed.
 *       @reply is set and should ALWAYS be released with bson_destroy().
 *
 *--------------------------------------------------------------------------
 */

bool
mongoc_cluster_run_command_monitored (mongoc_cluster_t *cluster,
                                      mongoc_cmd_parts_t *parts,
                                      mongoc_server_stream_t *server_stream,
                                      bson_t *reply,
                                      bson_error_t *error)
{
   int32_t compressor_id =
#ifdef MONGOC_ENABLE_COMPRESSION
      mongoc_server_description_compressor_id (server_stream->sd);
#else
      0;
#endif

   mongoc_cmd_parts_assemble (parts, server_stream);

   return mongoc_cluster_run_command_internal (cluster,
                                               &parts->assembled,
                                               server_stream->stream,
                                               compressor_id,
                                               true,
                                               &server_stream->sd->host,
                                               reply,
                                               error);
}


/*
 *--------------------------------------------------------------------------
 *
 * mongoc_cluster_run_command_private --
 *
 *       Internal function to run a command on a given stream.
 *       @error and @reply are optional out-pointers.
 *       The client's APM callbacks are not executed.
 *
 * Returns:
 *       true if successful; otherwise false and @error is set.
 *
 * Side effects:
 *       @reply is set and should ALWAYS be released with bson_destroy().
 *
 *--------------------------------------------------------------------------
 */

bool
mongoc_cluster_run_command_private (mongoc_cluster_t *cluster,
                                    mongoc_cmd_parts_t *parts,
                                    mongoc_stream_t *stream,
                                    uint32_t server_id,
                                    bson_t *reply,
                                    bson_error_t *error)
{
   mongoc_cmd_parts_assemble_simple (parts, server_id);

   /* monitored = false */
   return mongoc_cluster_run_command_internal (cluster,
                                               &parts->assembled,
                                               stream,
                                               0,
                                               /* not monitored */
                                               false,
                                               NULL,
                                               reply,
                                               error);
}

/*
 *--------------------------------------------------------------------------
 *
 * _mongoc_stream_run_ismaster --
 *
 *       Run an ismaster command on the given stream.
 *
 * Returns:
 *       A mongoc_server_description_t you must destroy. If the call failed
 *       its error is set and its type is MONGOC_SERVER_UNKNOWN.
 *
 *--------------------------------------------------------------------------
 */
static mongoc_server_description_t *
_mongoc_stream_run_ismaster (mongoc_cluster_t *cluster,
                             mongoc_stream_t *stream,
                             const char *address,
                             uint32_t server_id)
{
   const bson_t *command;
   mongoc_cmd_parts_t parts;
   bson_t reply;
   bson_error_t error = {0};
   int64_t start;
   int64_t rtt_msec;
   mongoc_server_description_t *sd;
   bool r;

   ENTRY;

   BSON_ASSERT (cluster);
   BSON_ASSERT (stream);

   command = _mongoc_topology_scanner_get_ismaster (
      cluster->client->topology->scanner);

   mongoc_cmd_parts_init (&parts, "admin", MONGOC_QUERY_SLAVE_OK, command);

   start = bson_get_monotonic_time ();
   mongoc_cluster_run_command_private (
      cluster, &parts, stream, server_id, &reply, &error);

   rtt_msec = (bson_get_monotonic_time () - start) / 1000;

   sd = (mongoc_server_description_t *) bson_malloc0 (
      sizeof (mongoc_server_description_t));

   mongoc_server_description_init (sd, address, server_id);
   /* send the error from run_command IN to handle_ismaster */
   mongoc_server_description_handle_ismaster (sd, &reply, rtt_msec, &error);

   bson_destroy (&reply);

   r = _mongoc_topology_update_from_handshake (cluster->client->topology, sd);
   if (!r) {
      mongoc_server_description_reset (sd);
      bson_set_error (&sd->error,
                      MONGOC_ERROR_STREAM,
                      MONGOC_ERROR_STREAM_NOT_ESTABLISHED,
                      "\"%s\" removed from topology",
                      address);
   }

   RETURN (sd);
}

/*
 *--------------------------------------------------------------------------
 *
 * _mongoc_cluster_run_ismaster --
 *
 *       Run an ismaster command for the given node and handle result.
 *
 * Returns:
 *       True if ismaster ran successfully, false otherwise.
 *
 * Side effects:
 *       Makes a blocking I/O call, updates cluster->topology->description
 *       with ismaster result.
 *
 *--------------------------------------------------------------------------
 */
static bool
_mongoc_cluster_run_ismaster (mongoc_cluster_t *cluster,
                              mongoc_cluster_node_t *node,
                              uint32_t server_id,
                              bson_error_t *error /* OUT */)
{
   bool r;
   mongoc_server_description_t *sd;

   ENTRY;

   BSON_ASSERT (cluster);
   BSON_ASSERT (node);
   BSON_ASSERT (node->stream);

   sd = _mongoc_stream_run_ismaster (
      cluster, node->stream, node->connection_address, server_id);

   if (sd->type == MONGOC_SERVER_UNKNOWN) {
      r = false;
      memcpy (error, &sd->error, sizeof (bson_error_t));
   } else {
      r = true;
      node->max_write_batch_size = sd->max_write_batch_size;
      node->min_wire_version = sd->min_wire_version;
      node->max_wire_version = sd->max_wire_version;
      node->max_bson_obj_size = sd->max_bson_obj_size;
      node->max_msg_size = sd->max_msg_size;
   }

   mongoc_server_description_destroy (sd);

   return r;
}


/*
 *--------------------------------------------------------------------------
 *
 * _mongoc_cluster_build_basic_auth_digest --
 *
 *       Computes the Basic Authentication digest using the credentials
 *       configured for @cluster and the @nonce provided.
 *
 *       The result should be freed by the caller using bson_free() when
 *       they are finished with it.
 *
 * Returns:
 *       A newly allocated string containing the digest.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

static char *
_mongoc_cluster_build_basic_auth_digest (mongoc_cluster_t *cluster,
                                         const char *nonce)
{
   const char *username;
   const char *password;
   char *password_digest;
   char *password_md5;
   char *digest_in;
   char *ret;

   ENTRY;

   /*
    * The following generates the digest to be used for basic authentication
    * with a MongoDB server. More information on the format can be found
    * at the following location:
    *
    * http://docs.mongodb.org/meta-driver/latest/legacy/
    *   implement-authentication-in-driver/
    */

   BSON_ASSERT (cluster);
   BSON_ASSERT (cluster->uri);

   username = mongoc_uri_get_username (cluster->uri);
   password = mongoc_uri_get_password (cluster->uri);
   password_digest = bson_strdup_printf ("%s:mongo:%s", username, password);
   password_md5 = _mongoc_hex_md5 (password_digest);
   digest_in = bson_strdup_printf ("%s%s%s", nonce, username, password_md5);
   ret = _mongoc_hex_md5 (digest_in);
   bson_free (digest_in);
   bson_free (password_md5);
   bson_free (password_digest);

   RETURN (ret);
}


/*
 *--------------------------------------------------------------------------
 *
 * _mongoc_cluster_auth_node_cr --
 *
 *       Performs authentication of @node using the credentials provided
 *       when configuring the @cluster instance.
 *
 *       This is the Challenge-Response mode of authentication.
 *
 * Returns:
 *       true if authentication was successful; otherwise false and
 *       @error is set.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

static bool
_mongoc_cluster_auth_node_cr (mongoc_cluster_t *cluster,
                              mongoc_stream_t *stream,
                              bson_error_t *error)
{
   mongoc_cmd_parts_t parts;
   bson_iter_t iter;
   const char *auth_source;
   bson_t command = {0};
   bson_t reply = {0};
   char *digest;
   char *nonce;
   bool ret;

   ENTRY;

   BSON_ASSERT (cluster);
   BSON_ASSERT (stream);

   if (!(auth_source = mongoc_uri_get_auth_source (cluster->uri)) ||
       (*auth_source == '\0')) {
      auth_source = "admin";
   }

   /*
    * To authenticate a node using basic authentication, we need to first
    * get the nonce from the server. We use that to hash our password which
    * is sent as a reply to the server. If everything went good we get a
    * success notification back from the server.
    */

   /*
    * Execute the getnonce command to fetch the nonce used for generating
    * md5 digest of our password information.
    */
   bson_init (&command);
   bson_append_int32 (&command, "getnonce", 8, 1);
   mongoc_cmd_parts_init (&parts, auth_source, MONGOC_QUERY_SLAVE_OK, &command);
   if (!mongoc_cluster_run_command_private (
          cluster, &parts, stream, 0, &reply, error)) {
      bson_destroy (&command);
      bson_destroy (&reply);
      RETURN (false);
   }
   bson_destroy (&command);
   if (!bson_iter_init_find_case (&iter, &reply, "nonce")) {
      bson_set_error (error,
                      MONGOC_ERROR_CLIENT,
                      MONGOC_ERROR_CLIENT_GETNONCE,
                      "Invalid reply from getnonce");
      bson_destroy (&reply);
      RETURN (false);
   }

   /*
    * Build our command to perform the authentication.
    */
   nonce = bson_iter_dup_utf8 (&iter, NULL);
   digest = _mongoc_cluster_build_basic_auth_digest (cluster, nonce);
   bson_init (&command);
   bson_append_int32 (&command, "authenticate", 12, 1);
   bson_append_utf8 (
      &command, "user", 4, mongoc_uri_get_username (cluster->uri), -1);
   bson_append_utf8 (&command, "nonce", 5, nonce, -1);
   bson_append_utf8 (&command, "key", 3, digest, -1);
   bson_destroy (&reply);
   bson_free (nonce);
   bson_free (digest);

   /*
    * Execute the authenticate command. mongoc_cluster_run_command_private
    * checks for {ok: 1} in the response.
    */
   mongoc_cmd_parts_cleanup (&parts);
   mongoc_cmd_parts_init (&parts, auth_source, MONGOC_QUERY_SLAVE_OK, &command);
   ret = mongoc_cluster_run_command_private (
      cluster, &parts, stream, 0, &reply, error);
   if (!ret) {
      /* error->message is already set */
      error->domain = MONGOC_ERROR_CLIENT;
      error->code = MONGOC_ERROR_CLIENT_AUTHENTICATE;
   }

   mongoc_cmd_parts_cleanup (&parts);
   bson_destroy (&command);
   bson_destroy (&reply);

   RETURN (ret);
}


/*
 *--------------------------------------------------------------------------
 *
 * _mongoc_cluster_auth_node_plain --
 *
 *       Perform SASL PLAIN authentication for @node. We do this manually
 *       instead of using the SASL module because its rather simplistic.
 *
 * Returns:
 *       true if successful; otherwise false and error is set.
 *
 * Side effects:
 *       error may be set.
 *
 *--------------------------------------------------------------------------
 */

static bool
_mongoc_cluster_auth_node_plain (mongoc_cluster_t *cluster,
                                 mongoc_stream_t *stream,
                                 bson_error_t *error)
{
   mongoc_cmd_parts_t parts;
   char buf[4096];
   int buflen = 0;
   const char *username;
   const char *password;
   bson_t b = BSON_INITIALIZER;
   bson_t reply;
   size_t len;
   char *str;
   bool ret;

   BSON_ASSERT (cluster);
   BSON_ASSERT (stream);

   username = mongoc_uri_get_username (cluster->uri);
   if (!username) {
      username = "";
   }

   password = mongoc_uri_get_password (cluster->uri);
   if (!password) {
      password = "";
   }

   str = bson_strdup_printf ("%c%s%c%s", '\0', username, '\0', password);
   len = strlen (username) + strlen (password) + 2;
   buflen = mongoc_b64_ntop ((const uint8_t *) str, len, buf, sizeof buf);
   bson_free (str);

   if (buflen == -1) {
      bson_set_error (error,
                      MONGOC_ERROR_CLIENT,
                      MONGOC_ERROR_CLIENT_AUTHENTICATE,
                      "failed base64 encoding message");
      return false;
   }

   BSON_APPEND_INT32 (&b, "saslStart", 1);
   BSON_APPEND_UTF8 (&b, "mechanism", "PLAIN");
   bson_append_utf8 (&b, "payload", 7, (const char *) buf, buflen);
   BSON_APPEND_INT32 (&b, "autoAuthorize", 1);

   mongoc_cmd_parts_init (&parts, "$external", MONGOC_QUERY_SLAVE_OK, &b);
   ret = mongoc_cluster_run_command_private (
      cluster, &parts, stream, 0, &reply, error);

   if (!ret) {
      /* error->message is already set */
      error->domain = MONGOC_ERROR_CLIENT;
      error->code = MONGOC_ERROR_CLIENT_AUTHENTICATE;
   }

   mongoc_cmd_parts_cleanup (&parts);
   bson_destroy (&b);
   bson_destroy (&reply);

   return ret;
}


#ifdef MONGOC_ENABLE_SSL
static bool
_mongoc_cluster_auth_node_x509 (mongoc_cluster_t *cluster,
                                mongoc_stream_t *stream,
                                bson_error_t *error)
{
   mongoc_cmd_parts_t parts;
   const char *username_from_uri = NULL;
   char *username_from_subject = NULL;
   bson_t cmd;
   bson_t reply;
   bool ret;

   BSON_ASSERT (cluster);
   BSON_ASSERT (stream);

   username_from_uri = mongoc_uri_get_username (cluster->uri);
   if (username_from_uri) {
      TRACE ("%s", "X509: got username from URI");
   } else {
      if (!cluster->client->ssl_opts.pem_file) {
         bson_set_error (error,
                         MONGOC_ERROR_CLIENT,
                         MONGOC_ERROR_CLIENT_AUTHENTICATE,
                         "cannot determine username for "
                         "X-509 authentication.");
         return false;
      }

      username_from_subject = mongoc_ssl_extract_subject (
         cluster->client->ssl_opts.pem_file, cluster->client->ssl_opts.pem_pwd);
      if (!username_from_subject) {
         bson_set_error (error,
                         MONGOC_ERROR_CLIENT,
                         MONGOC_ERROR_CLIENT_AUTHENTICATE,
                         "No username provided for X509 authentication.");
         return false;
      }

      TRACE ("%s", "X509: got username from certificate");
   }

   bson_init (&cmd);
   BSON_APPEND_INT32 (&cmd, "authenticate", 1);
   BSON_APPEND_UTF8 (&cmd, "mechanism", "MONGODB-X509");
   BSON_APPEND_UTF8 (&cmd,
                     "user",
                     username_from_uri ? username_from_uri
                                       : username_from_subject);

   mongoc_cmd_parts_init (&parts, "$external", MONGOC_QUERY_SLAVE_OK, &cmd);
   ret = mongoc_cluster_run_command_private (
      cluster, &parts, stream, 0, &reply, error);

   if (!ret) {
      /* error->message is already set */
      error->domain = MONGOC_ERROR_CLIENT;
      error->code = MONGOC_ERROR_CLIENT_AUTHENTICATE;
   }

   if (username_from_subject) {
      bson_free (username_from_subject);
   }

   mongoc_cmd_parts_cleanup (&parts);
   bson_destroy (&cmd);
   bson_destroy (&reply);

   return ret;
}
#endif


#ifdef MONGOC_ENABLE_CRYPTO
static bool
_mongoc_cluster_auth_node_scram (mongoc_cluster_t *cluster,
                                 mongoc_stream_t *stream,
                                 bson_error_t *error)
{
   mongoc_cmd_parts_t parts;
   uint32_t buflen = 0;
   mongoc_scram_t scram;
   bson_iter_t iter;
   bool ret = false;
   const char *tmpstr;
   const char *auth_source;
   uint8_t buf[4096] = {0};
   bson_t cmd;
   bson_t reply;
   int conv_id = 0;
   bson_subtype_t btype;

   BSON_ASSERT (cluster);
   BSON_ASSERT (stream);

   if (!(auth_source = mongoc_uri_get_auth_source (cluster->uri)) ||
       (*auth_source == '\0')) {
      auth_source = "admin";
   }

   _mongoc_scram_init (&scram);

   _mongoc_scram_set_pass (&scram, mongoc_uri_get_password (cluster->uri));
   _mongoc_scram_set_user (&scram, mongoc_uri_get_username (cluster->uri));
   if (*cluster->scram_client_key) {
      _mongoc_scram_set_client_key (
         &scram, cluster->scram_client_key, sizeof (cluster->scram_client_key));
   }
   if (*cluster->scram_server_key) {
      _mongoc_scram_set_server_key (
         &scram, cluster->scram_server_key, sizeof (cluster->scram_server_key));
   }
   if (*cluster->scram_salted_password) {
      _mongoc_scram_set_salted_password (
         &scram,
         cluster->scram_salted_password,
         sizeof (cluster->scram_salted_password));
   }

   for (;;) {
      if (!_mongoc_scram_step (
             &scram, buf, buflen, buf, sizeof buf, &buflen, error)) {
         goto failure;
      }

      bson_init (&cmd);

      if (scram.step == 1) {
         BSON_APPEND_INT32 (&cmd, "saslStart", 1);
         BSON_APPEND_UTF8 (&cmd, "mechanism", "SCRAM-SHA-1");
         bson_append_binary (
            &cmd, "payload", 7, BSON_SUBTYPE_BINARY, buf, buflen);
         BSON_APPEND_INT32 (&cmd, "autoAuthorize", 1);
      } else {
         BSON_APPEND_INT32 (&cmd, "saslContinue", 1);
         BSON_APPEND_INT32 (&cmd, "conversationId", conv_id);
         bson_append_binary (
            &cmd, "payload", 7, BSON_SUBTYPE_BINARY, buf, buflen);
      }

      TRACE ("SCRAM: authenticating (step %d)", scram.step);

      mongoc_cmd_parts_init (&parts, auth_source, MONGOC_QUERY_SLAVE_OK, &cmd);
      if (!mongoc_cluster_run_command_private (
             cluster, &parts, stream, 0, &reply, error)) {
         bson_destroy (&cmd);
         bson_destroy (&reply);

         /* error->message is already set */
         error->domain = MONGOC_ERROR_CLIENT;
         error->code = MONGOC_ERROR_CLIENT_AUTHENTICATE;
         goto failure;
      }

      mongoc_cmd_parts_cleanup (&parts);
      bson_destroy (&cmd);

      if (bson_iter_init_find (&iter, &reply, "done") &&
          bson_iter_as_bool (&iter)) {
         bson_destroy (&reply);
         break;
      }

      if (!bson_iter_init_find (&iter, &reply, "conversationId") ||
          !BSON_ITER_HOLDS_INT32 (&iter) ||
          !(conv_id = bson_iter_int32 (&iter)) ||
          !bson_iter_init_find (&iter, &reply, "payload") ||
          !BSON_ITER_HOLDS_BINARY (&iter)) {
         const char *errmsg =
            "Received invalid SCRAM reply from MongoDB server.";

         MONGOC_DEBUG ("SCRAM: authentication failed");

         if (bson_iter_init_find (&iter, &reply, "errmsg") &&
             BSON_ITER_HOLDS_UTF8 (&iter)) {
            errmsg = bson_iter_utf8 (&iter, NULL);
         }

         bson_set_error (error,
                         MONGOC_ERROR_CLIENT,
                         MONGOC_ERROR_CLIENT_AUTHENTICATE,
                         "%s",
                         errmsg);
         bson_destroy (&reply);
         goto failure;
      }

      bson_iter_binary (&iter, &btype, &buflen, (const uint8_t **) &tmpstr);

      if (buflen > sizeof buf) {
         bson_set_error (error,
                         MONGOC_ERROR_CLIENT,
                         MONGOC_ERROR_CLIENT_AUTHENTICATE,
                         "SCRAM reply from MongoDB is too large.");
         bson_destroy (&reply);
         goto failure;
      }

      memcpy (buf, tmpstr, buflen);

      bson_destroy (&reply);
   }

   TRACE ("%s", "SCRAM: authenticated");

   ret = true;
   memcpy (cluster->scram_client_key,
           scram.client_key,
           sizeof (cluster->scram_client_key));
   memcpy (cluster->scram_server_key,
           scram.server_key,
           sizeof (cluster->scram_server_key));
   memcpy (cluster->scram_salted_password,
           scram.salted_password,
           sizeof (cluster->scram_salted_password));

failure:
   _mongoc_scram_destroy (&scram);

   return ret;
}
#endif


/*
 *--------------------------------------------------------------------------
 *
 * _mongoc_cluster_auth_node --
 *
 *       Authenticate a cluster node depending on the required mechanism.
 *
 * Returns:
 *       true if authenticated. false on failure and @error is set.
 *
 * Side effects:
 *       @error is set on failure.
 *
 *--------------------------------------------------------------------------
 */

static bool
_mongoc_cluster_auth_node (mongoc_cluster_t *cluster,
                           mongoc_stream_t *stream,
                           const char *hostname,
                           int32_t max_wire_version,
                           bson_error_t *error)
{
   bool ret = false;
   const char *mechanism;
   ENTRY;

   BSON_ASSERT (cluster);
   BSON_ASSERT (stream);

   mechanism = mongoc_uri_get_auth_mechanism (cluster->uri);

   /* Use cached max_wire_version, not value from sd */
   if (!mechanism) {
      if (max_wire_version < WIRE_VERSION_SCRAM_DEFAULT) {
         mechanism = "MONGODB-CR";
      } else {
         mechanism = "SCRAM-SHA-1";
      }
   }

   if (0 == strcasecmp (mechanism, "MONGODB-CR")) {
      ret = _mongoc_cluster_auth_node_cr (cluster, stream, error);
   } else if (0 == strcasecmp (mechanism, "MONGODB-X509")) {
#ifdef MONGOC_ENABLE_SSL
      ret = _mongoc_cluster_auth_node_x509 (cluster, stream, error);
#else
      bson_set_error (error,
                      MONGOC_ERROR_CLIENT,
                      MONGOC_ERROR_CLIENT_AUTHENTICATE,
                      "The \"%s\" authentication mechanism requires libmongoc "
                      "built with --enable-ssl",
                      mechanism);
#endif
   } else if (0 == strcasecmp (mechanism, "SCRAM-SHA-1")) {
#ifdef MONGOC_ENABLE_CRYPTO
      ret = _mongoc_cluster_auth_node_scram (cluster, stream, error);
#else
      bson_set_error (error,
                      MONGOC_ERROR_CLIENT,
                      MONGOC_ERROR_CLIENT_AUTHENTICATE,
                      "The \"%s\" authentication mechanism requires libmongoc "
                      "built with --enable-ssl",
                      mechanism);
#endif
   } else if (0 == strcasecmp (mechanism, "GSSAPI")) {
#ifdef MONGOC_ENABLE_SASL
      ret = _mongoc_cluster_auth_node_sasl (cluster, stream, hostname, error);
#else
      bson_set_error (error,
                      MONGOC_ERROR_CLIENT,
                      MONGOC_ERROR_CLIENT_AUTHENTICATE,
                      "The \"%s\" authentication mechanism requires libmongoc "
                      "built with --enable-sasl",
                      mechanism);
#endif
   } else if (0 == strcasecmp (mechanism, "PLAIN")) {
      ret = _mongoc_cluster_auth_node_plain (cluster, stream, error);
   } else {
      bson_set_error (error,
                      MONGOC_ERROR_CLIENT,
                      MONGOC_ERROR_CLIENT_AUTHENTICATE,
                      "Unknown authentication mechanism \"%s\".",
                      mechanism);
   }

   if (!ret) {
      mongoc_counter_auth_failure_inc ();
      MONGOC_DEBUG ("Authentication failed: %s", error->message);
   } else {
      mongoc_counter_auth_success_inc ();
      TRACE ("%s", "Authentication succeeded");
   }

   RETURN (ret);
}


/*
 *--------------------------------------------------------------------------
 *
 * mongoc_cluster_disconnect_node --
 *
 *       Remove a node from the set of nodes. This should be done if
 *       a stream in the set is found to be invalid.
 *
 *       WARNING: pointers to a disconnected mongoc_cluster_node_t or
 *       its stream are now invalid, be careful of dangling pointers.
 *
 * Returns:
 *       None.
 *
 * Side effects:
 *       Removes node from cluster's set of nodes, and frees the
 *       mongoc_cluster_node_t if pooled.
 *
 *--------------------------------------------------------------------------
 */

void
mongoc_cluster_disconnect_node (mongoc_cluster_t *cluster, uint32_t server_id)
{
   mongoc_topology_t *topology = cluster->client->topology;
   ENTRY;

   if (topology->single_threaded) {
      mongoc_topology_scanner_node_t *scanner_node;

      scanner_node =
         mongoc_topology_scanner_get_node (topology->scanner, server_id);

      /* might never actually have connected */
      if (scanner_node && scanner_node->stream) {
         mongoc_topology_scanner_node_disconnect (scanner_node, true);
         EXIT;
      }
      EXIT;
   } else {
      mongoc_set_rm (cluster->nodes, server_id);
   }

   EXIT;
}

static void
_mongoc_cluster_node_destroy (mongoc_cluster_node_t *node)
{
   /* Failure, or Replica Set reconfigure without this node */
   mongoc_stream_failed (node->stream);
   bson_free (node->connection_address);

   bson_free (node);
}

static void
_mongoc_cluster_node_dtor (void *data_, void *ctx_)
{
   mongoc_cluster_node_t *node = (mongoc_cluster_node_t *) data_;

   _mongoc_cluster_node_destroy (node);
}

static mongoc_cluster_node_t *
_mongoc_cluster_node_new (mongoc_stream_t *stream,
                          const char *connection_address)
{
   mongoc_cluster_node_t *node;

   if (!stream) {
      return NULL;
   }

   node = (mongoc_cluster_node_t *) bson_malloc0 (sizeof *node);

   node->stream = stream;
   node->connection_address = bson_strdup (connection_address);
   node->timestamp = bson_get_monotonic_time ();

   node->max_wire_version = MONGOC_DEFAULT_WIRE_VERSION;
   node->min_wire_version = MONGOC_DEFAULT_WIRE_VERSION;

   node->max_write_batch_size = MONGOC_DEFAULT_WRITE_BATCH_SIZE;
   node->max_bson_obj_size = MONGOC_DEFAULT_BSON_OBJ_SIZE;
   node->max_msg_size = MONGOC_DEFAULT_MAX_MSG_SIZE;

   return node;
}

/*
 *--------------------------------------------------------------------------
 *
 * mongoc_cluster_add_node --
 *
 *       Add a new node to this cluster for the given server description.
 *
 *       NOTE: does NOT check if this server is already in the cluster.
 *
 * Returns:
 *       A stream connected to the server, or NULL on failure.
 *
 * Side effects:
 *       Adds a cluster node, or sets error on failure.
 *
 *--------------------------------------------------------------------------
 */
static mongoc_stream_t *
_mongoc_cluster_add_node (mongoc_cluster_t *cluster,
                          uint32_t server_id,
                          bson_error_t *error /* OUT */)
{
   mongoc_host_list_t *host = NULL;
   mongoc_cluster_node_t *cluster_node = NULL;
   mongoc_stream_t *stream;

   ENTRY;

   BSON_ASSERT (cluster);
   BSON_ASSERT (!cluster->client->topology->single_threaded);

   host =
      _mongoc_topology_host_by_id (cluster->client->topology, server_id, error);

   if (!host) {
      GOTO (error);
   }

   TRACE ("Adding new server to cluster: %s", host->host_and_port);

   stream = _mongoc_client_create_stream (cluster->client, host, error);

   if (!stream) {
      MONGOC_WARNING (
         "Failed connection to %s (%s)", host->host_and_port, error->message);
      GOTO (error);
   }

   /* take critical fields from a fresh ismaster */
   cluster_node = _mongoc_cluster_node_new (stream, host->host_and_port);

   if (!_mongoc_cluster_run_ismaster (
          cluster, cluster_node, server_id, error)) {
      GOTO (error);
   }

   if (cluster->requires_auth) {
      if (!_mongoc_cluster_auth_node (cluster,
                                      cluster_node->stream,
                                      host->host,
                                      cluster_node->max_wire_version,
                                      error)) {
         MONGOC_WARNING ("Failed authentication to %s (%s)",
                         host->host_and_port,
                         error->message);
         GOTO (error);
      }
   }

   mongoc_set_add (cluster->nodes, server_id, cluster_node);
   _mongoc_host_list_destroy_all (host);

   RETURN (stream);

error:
   _mongoc_host_list_destroy_all (host); /* null ok */

   if (cluster_node) {
      _mongoc_cluster_node_destroy (cluster_node); /* also destroys stream */
   }

   RETURN (NULL);
}

static void
node_not_found (mongoc_topology_t *topology,
                uint32_t server_id,
                bson_error_t *error /* OUT */)
{
   mongoc_server_description_t *sd;

   if (!error) {
      return;
   }

   sd = mongoc_topology_server_by_id (topology, server_id, error);

   if (!sd) {
      return;
   }

   if (sd->error.code) {
      memcpy (error, &sd->error, sizeof *error);
   } else {
      bson_set_error (error,
                      MONGOC_ERROR_STREAM,
                      MONGOC_ERROR_STREAM_NOT_ESTABLISHED,
                      "Could not find node %s",
                      sd->host.host_and_port);
   }

   mongoc_server_description_destroy (sd);
}

static void
stream_not_found (mongoc_topology_t *topology,
                  uint32_t server_id,
                  const char *connection_address,
                  bson_error_t *error /* OUT */)
{
   mongoc_server_description_t *sd;

   sd = mongoc_topology_server_by_id (topology, server_id, error);

   if (error) {
      if (sd && sd->error.code) {
         memcpy (error, &sd->error, sizeof *error);
      } else {
         bson_set_error (error,
                         MONGOC_ERROR_STREAM,
                         MONGOC_ERROR_STREAM_NOT_ESTABLISHED,
                         "Could not find stream for node %s",
                         connection_address);
      }
   }

   if (sd) {
      mongoc_server_description_destroy (sd);
   }
}

mongoc_server_stream_t *
_mongoc_cluster_stream_for_server (mongoc_cluster_t *cluster,
                                   uint32_t server_id,
                                   bool reconnect_ok,
                                   bson_error_t *error /* OUT */)
{
   mongoc_topology_t *topology;
   mongoc_server_stream_t *server_stream;
   bson_error_t err_local;
   /* if fetch_stream fails we need a place to receive error details and pass
    * them to mongoc_topology_invalidate_server. */
   bson_error_t *err_ptr = error ? error : &err_local;

   ENTRY;

   topology = cluster->client->topology;

   /* in the single-threaded use case we share topology's streams */
   if (topology->single_threaded) {
      server_stream = mongoc_cluster_fetch_stream_single (
         cluster, server_id, reconnect_ok, err_ptr);

   } else {
      server_stream = mongoc_cluster_fetch_stream_pooled (
         cluster, server_id, reconnect_ok, err_ptr);
   }

   if (!server_stream) {
      /* Server Discovery And Monitoring Spec: "When an application operation
       * fails because of any network error besides a socket timeout, the
       * client MUST replace the server's description with a default
       * ServerDescription of type Unknown, and fill the ServerDescription's
       * error field with useful information."
       *
       * error was filled by fetch_stream_single/pooled, pass it to invalidate()
       */
      mongoc_cluster_disconnect_node (cluster, server_id);
      mongoc_topology_invalidate_server (topology, server_id, err_ptr);
   }

   RETURN (server_stream);
}


/*
 *--------------------------------------------------------------------------
 *
 * mongoc_cluster_stream_for_server --
 *
 *       Fetch the stream for @server_id. If @reconnect_ok and there is no
 *       valid stream, attempts to reconnect; if not @reconnect_ok then only
 *       an existing stream can be returned, or NULL.
 *
 * Returns:
 *       A mongoc_server_stream_t, or NULL
 *
 * Side effects:
 *       May add a node or reconnect one, if @reconnect_ok.
 *       Authenticates the stream if needed.
 *       May set @error.
 *
 *--------------------------------------------------------------------------
 */

mongoc_server_stream_t *
mongoc_cluster_stream_for_server (mongoc_cluster_t *cluster,
                                  uint32_t server_id,
                                  bool reconnect_ok,
                                  bson_error_t *error)
{
   mongoc_server_stream_t *server_stream = NULL;

   ENTRY;

   BSON_ASSERT (cluster);
   BSON_ASSERT (server_id);

   server_stream = _mongoc_cluster_stream_for_server (
      cluster, server_id, reconnect_ok, error);

   if (!server_stream) {
      /* failed */
      mongoc_cluster_disconnect_node (cluster, server_id);
   }

   RETURN (server_stream);
}


static mongoc_server_stream_t *
mongoc_cluster_fetch_stream_single (mongoc_cluster_t *cluster,
                                    uint32_t server_id,
                                    bool reconnect_ok,
                                    bson_error_t *error /* OUT */)
{
   mongoc_topology_t *topology;
   mongoc_server_description_t *sd;
   mongoc_stream_t *stream;
   mongoc_topology_scanner_node_t *scanner_node;
   int64_t expire_at;

   topology = cluster->client->topology;
   scanner_node =
      mongoc_topology_scanner_get_node (topology->scanner, server_id);
   BSON_ASSERT (scanner_node && !scanner_node->retired);
   stream = scanner_node->stream;

   if (stream) {
      sd = mongoc_topology_server_by_id (topology, server_id, error);

      if (!sd) {
         return NULL;
      }
   } else {
      if (!reconnect_ok) {
         stream_not_found (
            topology, server_id, scanner_node->host.host_and_port, error);
         return NULL;
      }

      if (!mongoc_topology_scanner_node_setup (scanner_node, error)) {
         return NULL;
      }
      stream = scanner_node->stream;

      expire_at =
         bson_get_monotonic_time () + topology->connect_timeout_msec * 1000;
      if (!mongoc_stream_wait (stream, expire_at)) {
         bson_set_error (error,
                         MONGOC_ERROR_STREAM,
                         MONGOC_ERROR_STREAM_CONNECT,
                         "Failed to connect to target host: '%s'",
                         scanner_node->host.host_and_port);
         return NULL;
      }

#ifdef MONGOC_ENABLE_SSL
      if (cluster->client->use_ssl) {
         bool r;
         mongoc_stream_t *tls_stream;

         for (tls_stream = stream; tls_stream->type != MONGOC_STREAM_TLS;
              tls_stream = mongoc_stream_get_base_stream (tls_stream)) {
         }

         r = mongoc_stream_tls_handshake_block (
            tls_stream,
            scanner_node->host.host,
            (int32_t) topology->connect_timeout_msec * 1000,
            error);

         if (!r) {
            mongoc_topology_scanner_node_disconnect (scanner_node, true);
            return NULL;
         }
      }
#endif

      sd = _mongoc_stream_run_ismaster (
         cluster, stream, scanner_node->host.host_and_port, server_id);
   }

   if (sd->type == MONGOC_SERVER_UNKNOWN) {
      memcpy (error, &sd->error, sizeof *error);
      mongoc_server_description_destroy (sd);
      return NULL;
   }

   /* stream open but not auth'ed: first use since connect or reconnect */
   if (cluster->requires_auth && !scanner_node->has_auth) {
      if (!_mongoc_cluster_auth_node (cluster,
                                      stream,
                                      sd->host.host,
                                      sd->max_wire_version,
                                      &sd->error)) {
         memcpy (error, &sd->error, sizeof *error);
         mongoc_server_description_destroy (sd);
         return NULL;
      }

      scanner_node->has_auth = true;
   }

   return mongoc_server_stream_new (topology->description.type, sd, stream);
}


static mongoc_server_stream_t *
_mongoc_cluster_create_server_stream (mongoc_topology_t *topology,
                                      uint32_t server_id,
                                      mongoc_stream_t *stream,
                                      bson_error_t *error /* OUT */)
{
   mongoc_server_description_t *sd;

   sd = mongoc_topology_server_by_id (topology, server_id, error);

   if (!sd) {
      return NULL;
   }

   return mongoc_server_stream_new (
      _mongoc_topology_get_type (topology), sd, stream);
}


static mongoc_server_stream_t *
mongoc_cluster_fetch_stream_pooled (mongoc_cluster_t *cluster,
                                    uint32_t server_id,
                                    bool reconnect_ok,
                                    bson_error_t *error /* OUT */)
{
   mongoc_topology_t *topology;
   mongoc_stream_t *stream;
   mongoc_cluster_node_t *cluster_node;
   int64_t timestamp;

   cluster_node =
      (mongoc_cluster_node_t *) mongoc_set_get (cluster->nodes, server_id);

   topology = cluster->client->topology;

   if (cluster_node) {
      BSON_ASSERT (cluster_node->stream);

      timestamp = mongoc_topology_server_timestamp (topology, server_id);
      if (timestamp == -1 || cluster_node->timestamp < timestamp) {
         /* topology change or net error during background scan made us remove
          * or replace server description since node's birth. destroy node. */
         mongoc_cluster_disconnect_node (cluster, server_id);
      } else {
         return _mongoc_cluster_create_server_stream (
            topology, server_id, cluster_node->stream, error);
      }
   }

   /* no node, or out of date */
   if (!reconnect_ok) {
      node_not_found (topology, server_id, error);
      return NULL;
   }

   stream = _mongoc_cluster_add_node (cluster, server_id, error);
   if (stream) {
      return _mongoc_cluster_create_server_stream (
         topology, server_id, stream, error);
   } else {
      return NULL;
   }
}

/*
 *--------------------------------------------------------------------------
 *
 * mongoc_cluster_init --
 *
 *       Initializes @cluster using the @uri and @client provided. The
 *       @uri is used to determine the "mode" of the cluster. Based on the
 *       uri we can determine if we are connected to a single host, a
 *       replicaSet, or a shardedCluster.
 *
 * Returns:
 *       None.
 *
 * Side effects:
 *       @cluster is initialized.
 *
 *--------------------------------------------------------------------------
 */

void
mongoc_cluster_init (mongoc_cluster_t *cluster,
                     const mongoc_uri_t *uri,
                     void *client)
{
   ENTRY;

   BSON_ASSERT (cluster);
   BSON_ASSERT (uri);

   memset (cluster, 0, sizeof *cluster);

   cluster->uri = mongoc_uri_copy (uri);
   cluster->client = (mongoc_client_t *) client;
   cluster->requires_auth =
      (mongoc_uri_get_username (uri) || mongoc_uri_get_auth_mechanism (uri));

   cluster->sockettimeoutms = mongoc_uri_get_option_as_int32 (
      uri, MONGOC_URI_SOCKETTIMEOUTMS, MONGOC_DEFAULT_SOCKETTIMEOUTMS);

   cluster->socketcheckintervalms =
      mongoc_uri_get_option_as_int32 (uri,
                                      MONGOC_URI_SOCKETCHECKINTERVALMS,
                                      MONGOC_TOPOLOGY_SOCKET_CHECK_INTERVAL_MS);

   /* TODO for single-threaded case we don't need this */
   cluster->nodes = mongoc_set_new (8, _mongoc_cluster_node_dtor, NULL);

   _mongoc_array_init (&cluster->iov, sizeof (mongoc_iovec_t));

   cluster->operation_id = rand ();

   EXIT;
}

/*
 *--------------------------------------------------------------------------
 *
 * mongoc_cluster_destroy --
 *
 *       Clean up after @cluster and destroy all active connections.
 *       All resources for @cluster are released.
 *
 * Returns:
 *       None.
 *
 * Side effects:
 *       Everything.
 *
 *--------------------------------------------------------------------------
 */

void
mongoc_cluster_destroy (mongoc_cluster_t *cluster) /* INOUT */
{
   ENTRY;

   BSON_ASSERT (cluster);

   mongoc_uri_destroy (cluster->uri);

   mongoc_set_destroy (cluster->nodes);

   _mongoc_array_destroy (&cluster->iov);

   EXIT;
}


/*
 *--------------------------------------------------------------------------
 *
 * mongoc_cluster_stream_for_optype --
 *
 *       Internal server selection.
 *
 * Returns:
 *       A mongoc_server_stream_t on which you must call
 *       mongoc_server_stream_cleanup, or NULL on failure (sets @error)
 *
 * Side effects:
 *       May set @error.
 *       May add new nodes to @cluster->nodes.
 *
 *--------------------------------------------------------------------------
 */

static mongoc_server_stream_t *
_mongoc_cluster_stream_for_optype (mongoc_cluster_t *cluster,
                                   mongoc_ss_optype_t optype,
                                   const mongoc_read_prefs_t *read_prefs,
                                   bson_error_t *error)
{
   mongoc_server_stream_t *server_stream;
   uint32_t server_id;
   mongoc_topology_t *topology = cluster->client->topology;

   ENTRY;

   BSON_ASSERT (cluster);

   server_id =
      mongoc_topology_select_server_id (topology, optype, read_prefs, error);

   if (!server_id) {
      RETURN (NULL);
   }

   if (!mongoc_cluster_check_interval (cluster, server_id)) {
      /* Server Selection Spec: try once more */
      server_id =
         mongoc_topology_select_server_id (topology, optype, read_prefs, error);

      if (!server_id) {
         RETURN (NULL);
      }
   }

   /* connect or reconnect to server if necessary */
   server_stream = _mongoc_cluster_stream_for_server (
      cluster, server_id, true /* reconnect_ok */, error);

   RETURN (server_stream);
}


/*
 *--------------------------------------------------------------------------
 *
 * mongoc_cluster_stream_for_reads --
 *
 *       Internal server selection.
 *
 * Returns:
 *       A mongoc_server_stream_t on which you must call
 *       mongoc_server_stream_cleanup, or NULL on failure (sets @error)
 *
 * Side effects:
 *       May set @error.
 *       May add new nodes to @cluster->nodes.
 *
 *--------------------------------------------------------------------------
 */

mongoc_server_stream_t *
mongoc_cluster_stream_for_reads (mongoc_cluster_t *cluster,
                                 const mongoc_read_prefs_t *read_prefs,
                                 bson_error_t *error)
{
   return _mongoc_cluster_stream_for_optype (
      cluster, MONGOC_SS_READ, read_prefs, error);
}

/*
 *--------------------------------------------------------------------------
 *
 * mongoc_cluster_stream_for_writes --
 *
 *       Get a stream for write operations.
 *
 * Returns:
 *       A mongoc_server_stream_t on which you must call
 *       mongoc_server_stream_cleanup, or NULL on failure (sets @error)
 *
 * Side effects:
 *       May set @error.
 *       May add new nodes to @cluster->nodes.
 *
 *--------------------------------------------------------------------------
 */

mongoc_server_stream_t *
mongoc_cluster_stream_for_writes (mongoc_cluster_t *cluster,
                                  bson_error_t *error)
{
   return _mongoc_cluster_stream_for_optype (
      cluster, MONGOC_SS_WRITE, NULL, error);
}

static bool
_mongoc_cluster_min_of_max_obj_size_sds (void *item, void *ctx)
{
   mongoc_server_description_t *sd = (mongoc_server_description_t *) item;
   int32_t *current_min = (int32_t *) ctx;

   if (sd->max_bson_obj_size < *current_min) {
      *current_min = sd->max_bson_obj_size;
   }
   return true;
}

static bool
_mongoc_cluster_min_of_max_obj_size_nodes (void *item, void *ctx)
{
   mongoc_cluster_node_t *node = (mongoc_cluster_node_t *) item;
   int32_t *current_min = (int32_t *) ctx;

   if (node->max_bson_obj_size < *current_min) {
      *current_min = node->max_bson_obj_size;
   }
   return true;
}

static bool
_mongoc_cluster_min_of_max_msg_size_sds (void *item, void *ctx)
{
   mongoc_server_description_t *sd = (mongoc_server_description_t *) item;
   int32_t *current_min = (int32_t *) ctx;

   if (sd->max_msg_size < *current_min) {
      *current_min = sd->max_msg_size;
   }
   return true;
}

static bool
_mongoc_cluster_min_of_max_msg_size_nodes (void *item, void *ctx)
{
   mongoc_cluster_node_t *node = (mongoc_cluster_node_t *) item;
   int32_t *current_min = (int32_t *) ctx;

   if (node->max_msg_size < *current_min) {
      *current_min = node->max_msg_size;
   }
   return true;
}

/*
 *--------------------------------------------------------------------------
 *
 * mongoc_cluster_get_max_bson_obj_size --
 *
 *      Return the minimum max_bson_obj_size across all servers in cluster.
 *
 *      NOTE: this method uses the topology's mutex.
 *
 * Returns:
 *      The minimum max_bson_obj_size.
 *
 * Side effects:
 *      None
 *
 *--------------------------------------------------------------------------
 */
int32_t
mongoc_cluster_get_max_bson_obj_size (mongoc_cluster_t *cluster)
{
   int32_t max_bson_obj_size = -1;

   max_bson_obj_size = MONGOC_DEFAULT_BSON_OBJ_SIZE;

   if (!cluster->client->topology->single_threaded) {
      mongoc_set_for_each (cluster->nodes,
                           _mongoc_cluster_min_of_max_obj_size_nodes,
                           &max_bson_obj_size);
   } else {
      mongoc_set_for_each (cluster->client->topology->description.servers,
                           _mongoc_cluster_min_of_max_obj_size_sds,
                           &max_bson_obj_size);
   }

   return max_bson_obj_size;
}


/*
 *--------------------------------------------------------------------------
 *
 * mongoc_cluster_get_max_msg_size --
 *
 *      Return the minimum max msg size across all servers in cluster.
 *
 *      NOTE: this method uses the topology's mutex.
 *
 * Returns:
 *      The minimum max_msg_size
 *
 * Side effects:
 *      None
 *
 *--------------------------------------------------------------------------
 */
int32_t
mongoc_cluster_get_max_msg_size (mongoc_cluster_t *cluster)
{
   int32_t max_msg_size = MONGOC_DEFAULT_MAX_MSG_SIZE;

   if (!cluster->client->topology->single_threaded) {
      mongoc_set_for_each (cluster->nodes,
                           _mongoc_cluster_min_of_max_msg_size_nodes,
                           &max_msg_size);
   } else {
      mongoc_set_for_each (cluster->client->topology->description.servers,
                           _mongoc_cluster_min_of_max_msg_size_sds,
                           &max_msg_size);
   }

   return max_msg_size;
}


/*
 *--------------------------------------------------------------------------
 *
 * mongoc_cluster_check_interval --
 *
 *      Server Selection Spec:
 *
 *      Only for single-threaded drivers.
 *
 *      If a server is selected that has an existing connection that has been
 *      idle for socketCheckIntervalMS, the driver MUST check the connection
 *      with the "ping" command. If the ping succeeds, use the selected
 *      connection. If not, set the server's type to Unknown and update the
 *      Topology Description according to the Server Discovery and Monitoring
 *      Spec, and attempt once more to select a server.
 *
 * Returns:
 *      True if the check succeeded or no check was required, false if the
 *      check failed.
 *
 * Side effects:
 *      If a check fails, closes stream and may set server type Unknown.
 *
 *--------------------------------------------------------------------------
 */

bool
mongoc_cluster_check_interval (mongoc_cluster_t *cluster, uint32_t server_id)
{
   mongoc_cmd_parts_t parts;
   mongoc_topology_t *topology;
   mongoc_topology_scanner_node_t *scanner_node;
   mongoc_stream_t *stream;
   int64_t now;
   bson_t command;
   bson_error_t error;
   bool r = true;

   topology = cluster->client->topology;

   if (!topology->single_threaded) {
      return true;
   }

   scanner_node =
      mongoc_topology_scanner_get_node (topology->scanner, server_id);

   if (!scanner_node) {
      return false;
   }

   BSON_ASSERT (!scanner_node->retired);

   stream = scanner_node->stream;

   if (!stream) {
      return false;
   }

   now = bson_get_monotonic_time ();

   if (scanner_node->last_used + (1000 * CHECK_CLOSED_DURATION_MSEC) < now) {
      if (mongoc_stream_check_closed (stream)) {
         mongoc_cluster_disconnect_node (cluster, server_id);
         return false;
      }
   }

   if (scanner_node->last_used + (1000 * cluster->socketcheckintervalms) <
       now) {
      bson_init (&command);
      BSON_APPEND_INT32 (&command, "ping", 1);
      mongoc_cmd_parts_init (&parts, "admin", MONGOC_QUERY_SLAVE_OK, &command);
      r = mongoc_cluster_run_command_private (
         cluster, &parts, stream, server_id, NULL, &error /* OUT */);

      mongoc_cmd_parts_cleanup (&parts);
      bson_destroy (&command);

      if (!r) {
         mongoc_cluster_disconnect_node (cluster, server_id);
         mongoc_topology_invalidate_server (
            topology, server_id, &error /* IN */);
      }
   }

   return r;
}


/*
 *--------------------------------------------------------------------------
 *
 * mongoc_cluster_sendv_to_server --
 *
 *       Sends the given RPCs to the given server.
 *
 * Returns:
 *       True if successful.
 *
 * Side effects:
 *       @rpc may be mutated and should be considered invalid after calling
 *       this method.
 *
 *       @error may be set.
 *
 *--------------------------------------------------------------------------
 */

bool
mongoc_cluster_sendv_to_server (mongoc_cluster_t *cluster,
                                mongoc_rpc_t *rpc,
                                mongoc_server_stream_t *server_stream,
                                const mongoc_write_concern_t *write_concern,
                                bson_error_t *error)
{
   uint32_t server_id;
   mongoc_topology_scanner_node_t *scanner_node;
   const bson_t *b;
   mongoc_rpc_t gle;
   bool need_gle;
   char cmdname[140];
   int32_t max_msg_size;
   bool ret = false;
#ifdef MONGOC_ENABLE_COMPRESSION
   int32_t compressor_id = 0;
   char *output = NULL;
#endif

   ENTRY;

   BSON_ASSERT (cluster);
   BSON_ASSERT (rpc);
   BSON_ASSERT (server_stream);

   server_id = server_stream->sd->id;

   if (cluster->client->in_exhaust) {
      bson_set_error (error,
                      MONGOC_ERROR_CLIENT,
                      MONGOC_ERROR_CLIENT_IN_EXHAUST,
                      "A cursor derived from this client is in exhaust.");
      GOTO (done);
   }

   if (!write_concern) {
      write_concern = cluster->client->write_concern;
   }

   _mongoc_array_clear (&cluster->iov);
#ifdef MONGOC_ENABLE_COMPRESSION
   compressor_id = mongoc_server_description_compressor_id (server_stream->sd);
#endif

   need_gle = _mongoc_rpc_needs_gle (rpc, write_concern);
   _mongoc_cluster_inc_egress_rpc (rpc);
   _mongoc_rpc_gather (rpc, &cluster->iov);
   _mongoc_rpc_swab_to_le (rpc);

#ifdef MONGOC_ENABLE_COMPRESSION
   if (compressor_id) {
      output = _mongoc_rpc_compress (cluster, compressor_id, rpc, error);
      if (output == NULL) {
         GOTO (done);
      }
   }
#endif

   max_msg_size = mongoc_server_stream_max_msg_size (server_stream);

   if (BSON_UINT32_FROM_LE (rpc->header.msg_len) > max_msg_size) {
      bson_set_error (error,
                      MONGOC_ERROR_CLIENT,
                      MONGOC_ERROR_CLIENT_TOO_BIG,
                      "Attempted to send an RPC larger than the "
                      "max allowed message size. Was %u, allowed %u.",
                      BSON_UINT32_FROM_LE (rpc->header.msg_len),
                      max_msg_size);
      GOTO (done);
   }

   if (need_gle) {
      gle.header.msg_len = 0;
      gle.header.request_id = ++cluster->request_id;
      gle.header.response_to = 0;
      gle.header.opcode = MONGOC_OPCODE_QUERY;
      gle.query.flags = MONGOC_QUERY_NONE;

      switch (BSON_UINT32_FROM_LE (rpc->header.opcode)) {
      case MONGOC_OPCODE_INSERT:
         DB_AND_CMD_FROM_COLLECTION (cmdname, rpc->insert.collection);
         break;
      case MONGOC_OPCODE_DELETE:
         DB_AND_CMD_FROM_COLLECTION (cmdname, rpc->delete_.collection);
         break;
      case MONGOC_OPCODE_UPDATE:
         DB_AND_CMD_FROM_COLLECTION (cmdname, rpc->update.collection);
         break;
      default:
         BSON_ASSERT (false);
         DB_AND_CMD_FROM_COLLECTION (cmdname, "admin.$cmd");
         break;
      }

      gle.query.collection = cmdname;
      gle.query.skip = 0;
      gle.query.n_return = 1;
      b = _mongoc_write_concern_get_gle (
         (mongoc_write_concern_t *) write_concern);
      gle.query.query = bson_get_data (b);
      gle.query.fields = NULL;
      _mongoc_rpc_gather (&gle, &cluster->iov);
      _mongoc_rpc_swab_to_le (&gle);
   }

   if (!_mongoc_stream_writev_full (server_stream->stream,
                                    cluster->iov.data,
                                    cluster->iov.len,
                                    cluster->sockettimeoutms,
                                    error)) {
      GOTO (done);
   }

   if (cluster->client->topology->single_threaded) {
      scanner_node = mongoc_topology_scanner_get_node (
         cluster->client->topology->scanner, server_id);

      if (scanner_node) {
         scanner_node->last_used = bson_get_monotonic_time ();
      }
   }

   ret = true;

done:

#ifdef MONGOC_ENABLE_COMPRESSION
   if (compressor_id) {
      bson_free (output);
   }
#endif

   RETURN (ret);
}


/*
 *--------------------------------------------------------------------------
 *
 * mongoc_cluster_try_recv --
 *
 *       Tries to receive the next event from the MongoDB server.
 *       The contents are loaded into @buffer and then
 *       scattered into the @rpc structure. @rpc is valid as long as
 *       @buffer contains the contents read into it.
 *
 *       Callers that can optimize a reuse of @buffer should do so. It
 *       can save many memory allocations.
 *
 * Returns:
 *       True if successful.
 *
 * Side effects:
 *       @rpc is set on success, @error on failure.
 *       @buffer will be filled with the input data.
 *
 *--------------------------------------------------------------------------
 */

bool
mongoc_cluster_try_recv (mongoc_cluster_t *cluster,
                         mongoc_rpc_t *rpc,
                         mongoc_buffer_t *buffer,
                         mongoc_server_stream_t *server_stream,
                         bson_error_t *error)
{
   uint32_t server_id;
   int32_t msg_len;
   int32_t max_msg_size;
   off_t pos;

   ENTRY;

   BSON_ASSERT (cluster);
   BSON_ASSERT (rpc);
   BSON_ASSERT (buffer);
   BSON_ASSERT (server_stream);

   server_id = server_stream->sd->id;

   TRACE ("Waiting for reply from server_id \"%u\"", server_id);

   /*
    * Buffer the message length to determine how much more to read.
    */
   pos = buffer->len;
   if (!_mongoc_buffer_append_from_stream (
          buffer, server_stream->stream, 4, cluster->sockettimeoutms, error)) {
      MONGOC_DEBUG (
         "Could not read 4 bytes, stream probably closed or timed out");
      mongoc_counter_protocol_ingress_error_inc ();
      mongoc_cluster_disconnect_node (cluster, server_id);
      RETURN (false);
   }

   /*
    * Read the msg length from the buffer.
    */
   memcpy (&msg_len, &buffer->data[buffer->off + pos], 4);
   msg_len = BSON_UINT32_FROM_LE (msg_len);
   max_msg_size = mongoc_server_stream_max_msg_size (server_stream);
   if ((msg_len < 16) || (msg_len > max_msg_size)) {
      bson_set_error (error,
                      MONGOC_ERROR_PROTOCOL,
                      MONGOC_ERROR_PROTOCOL_INVALID_REPLY,
                      "Corrupt or malicious reply received.");
      mongoc_cluster_disconnect_node (cluster, server_id);
      mongoc_counter_protocol_ingress_error_inc ();
      RETURN (false);
   }

   /*
    * Read the rest of the message from the stream.
    */
   if (!_mongoc_buffer_append_from_stream (buffer,
                                           server_stream->stream,
                                           msg_len - 4,
                                           cluster->sockettimeoutms,
                                           error)) {
      mongoc_cluster_disconnect_node (cluster, server_id);
      mongoc_counter_protocol_ingress_error_inc ();
      RETURN (false);
   }

   /*
    * Scatter the buffer into the rpc structure.
    */
   if (!_mongoc_rpc_scatter (rpc, &buffer->data[buffer->off + pos], msg_len)) {
      bson_set_error (error,
                      MONGOC_ERROR_PROTOCOL,
                      MONGOC_ERROR_PROTOCOL_INVALID_REPLY,
                      "Failed to decode reply from server.");
      mongoc_cluster_disconnect_node (cluster, server_id);
      mongoc_counter_protocol_ingress_error_inc ();
      RETURN (false);
   }

   if (BSON_UINT32_FROM_LE (rpc->header.opcode) == MONGOC_OPCODE_COMPRESSED) {
      uint8_t *buf = NULL;
      size_t len = BSON_UINT32_FROM_LE (rpc->compressed.uncompressed_size) +
                   sizeof (mongoc_rpc_header_t);

      buf = bson_malloc0 (len);
      if (!_mongoc_rpc_decompress (rpc, buf, len)) {
         bson_free (buf);
         bson_set_error (error,
                         MONGOC_ERROR_PROTOCOL,
                         MONGOC_ERROR_PROTOCOL_INVALID_REPLY,
                         "Could not decompress server reply");
         RETURN (false);
      }

      _mongoc_buffer_destroy (buffer);
      _mongoc_buffer_init (buffer, buf, len, NULL, NULL);
   }
   _mongoc_rpc_swab_from_le (rpc);

   _mongoc_cluster_inc_ingress_rpc (rpc);

   RETURN (true);
}
