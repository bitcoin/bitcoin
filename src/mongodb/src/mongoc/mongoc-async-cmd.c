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

#include "mongoc-client.h"
#include "mongoc-async-cmd-private.h"
#include "mongoc-async-private.h"
#include "mongoc-error.h"
#include "mongoc-opcode.h"
#include "mongoc-rpc-private.h"
#include "mongoc-stream-private.h"
#include "mongoc-server-description-private.h"
#include "mongoc-log.h"
#include "utlist.h"

#ifdef MONGOC_ENABLE_SSL
#include "mongoc-stream-tls.h"
#endif

#undef MONGOC_LOG_DOMAIN
#define MONGOC_LOG_DOMAIN "async"

typedef mongoc_async_cmd_result_t (*_mongoc_async_cmd_phase_t) (
   mongoc_async_cmd_t *cmd);

mongoc_async_cmd_result_t
_mongoc_async_cmd_phase_setup (mongoc_async_cmd_t *cmd);
mongoc_async_cmd_result_t
_mongoc_async_cmd_phase_send (mongoc_async_cmd_t *cmd);
mongoc_async_cmd_result_t
_mongoc_async_cmd_phase_recv_len (mongoc_async_cmd_t *cmd);
mongoc_async_cmd_result_t
_mongoc_async_cmd_phase_recv_rpc (mongoc_async_cmd_t *cmd);

static const _mongoc_async_cmd_phase_t gMongocCMDPhases[] = {
   _mongoc_async_cmd_phase_setup,
   _mongoc_async_cmd_phase_send,
   _mongoc_async_cmd_phase_recv_len,
   _mongoc_async_cmd_phase_recv_rpc,
   NULL, /* no callback for MONGOC_ASYNC_CMD_ERROR_STATE    */
   NULL, /* no callback for MONGOC_ASYNC_CMD_CANCELED_STATE */
};

#ifdef MONGOC_ENABLE_SSL
int
mongoc_async_cmd_tls_setup (mongoc_stream_t *stream,
                            int *events,
                            void *ctx,
                            int32_t timeout_msec,
                            bson_error_t *error)
{
   mongoc_stream_t *tls_stream;
   const char *host = (const char *) ctx;
   int retry_events = 0;

   for (tls_stream = stream; tls_stream->type != MONGOC_STREAM_TLS;
        tls_stream = mongoc_stream_get_base_stream (tls_stream)) {
   }

   if (mongoc_stream_tls_handshake (
          tls_stream, host, timeout_msec, &retry_events, error)) {
      return 1;
   }

   if (retry_events) {
      *events = retry_events;
      return 0;
   }
   return -1;
}
#endif

bool
mongoc_async_cmd_run (mongoc_async_cmd_t *acmd)
{
   mongoc_async_cmd_result_t result;
   int64_t rtt_msec;
   _mongoc_async_cmd_phase_t phase_callback;

   phase_callback = gMongocCMDPhases[acmd->state];
   if (phase_callback) {
      result = phase_callback (acmd);
   } else {
      result = MONGOC_ASYNC_CMD_ERROR;
   }

   if (result == MONGOC_ASYNC_CMD_IN_PROGRESS) {
      return true;
   }

   rtt_msec = (bson_get_monotonic_time () - acmd->cmd_started) / 1000;

   if (result == MONGOC_ASYNC_CMD_SUCCESS) {
      acmd->cb (result, &acmd->reply, rtt_msec, acmd->data, &acmd->error);
   } else {
      /* we're in ERROR, TIMEOUT, or CANCELED */
      acmd->cb (result, NULL, rtt_msec, acmd->data, &acmd->error);
   }

   mongoc_async_cmd_destroy (acmd);
   return false;
}

void
_mongoc_async_cmd_init_send (mongoc_async_cmd_t *acmd, const char *dbname)
{
   bson_snprintf (acmd->ns, sizeof acmd->ns, "%s.$cmd", dbname);

   acmd->rpc.header.msg_len = 0;
   acmd->rpc.header.request_id = ++acmd->async->request_id;
   acmd->rpc.header.response_to = 0;
   acmd->rpc.header.opcode = MONGOC_OPCODE_QUERY;
   acmd->rpc.query.flags = MONGOC_QUERY_SLAVE_OK;
   acmd->rpc.query.collection = acmd->ns;
   acmd->rpc.query.skip = 0;
   acmd->rpc.query.n_return = -1;
   acmd->rpc.query.query = bson_get_data (&acmd->cmd);
   acmd->rpc.query.fields = NULL;

   /* This will always be isMaster, which are not allowed to be compressed */
   _mongoc_rpc_gather (&acmd->rpc, &acmd->array);
   acmd->iovec = (mongoc_iovec_t *) acmd->array.data;
   acmd->niovec = acmd->array.len;
   _mongoc_rpc_swab_to_le (&acmd->rpc);
}

void
_mongoc_async_cmd_state_start (mongoc_async_cmd_t *acmd)
{
   if (acmd->setup) {
      acmd->state = MONGOC_ASYNC_CMD_SETUP;
   } else {
      acmd->state = MONGOC_ASYNC_CMD_SEND;
   }

   acmd->events = POLLOUT;
}

mongoc_async_cmd_t *
mongoc_async_cmd_new (mongoc_async_t *async,
                      mongoc_stream_t *stream,
                      mongoc_async_cmd_setup_t setup,
                      void *setup_ctx,
                      const char *dbname,
                      const bson_t *cmd,
                      mongoc_async_cmd_cb_t cb,
                      void *cb_data,
                      int64_t timeout_msec)
{
   mongoc_async_cmd_t *acmd;

   BSON_ASSERT (cmd);
   BSON_ASSERT (dbname);
   BSON_ASSERT (stream);

   acmd = (mongoc_async_cmd_t *) bson_malloc0 (sizeof (*acmd));
   acmd->async = async;
   acmd->timeout_msec = timeout_msec;
   acmd->stream = stream;
   acmd->setup = setup;
   acmd->setup_ctx = setup_ctx;
   acmd->cb = cb;
   acmd->data = cb_data;
   acmd->connect_started = bson_get_monotonic_time ();
   bson_copy_to (cmd, &acmd->cmd);

   _mongoc_array_init (&acmd->array, sizeof (mongoc_iovec_t));
   _mongoc_buffer_init (&acmd->buffer, NULL, 0, NULL, NULL);

   _mongoc_async_cmd_init_send (acmd, dbname);

   _mongoc_async_cmd_state_start (acmd);

   async->ncmds++;
   DL_APPEND (async->cmds, acmd);

   return acmd;
}


void
mongoc_async_cmd_destroy (mongoc_async_cmd_t *acmd)
{
   BSON_ASSERT (acmd);

   DL_DELETE (acmd->async->cmds, acmd);
   acmd->async->ncmds--;

   bson_destroy (&acmd->cmd);

   if (acmd->reply_needs_cleanup) {
      bson_destroy (&acmd->reply);
   }

   _mongoc_array_destroy (&acmd->array);
   _mongoc_buffer_destroy (&acmd->buffer);

   bson_free (acmd);
}

mongoc_async_cmd_result_t
_mongoc_async_cmd_phase_setup (mongoc_async_cmd_t *acmd)
{
   int retval;

   BSON_ASSERT (acmd->timeout_msec < INT32_MAX);
   retval = acmd->setup (acmd->stream,
                         &acmd->events,
                         acmd->setup_ctx,
                         (int32_t) acmd->timeout_msec,
                         &acmd->error);
   switch (retval) {
   case -1:
      return MONGOC_ASYNC_CMD_ERROR;
   case 0:
      break;
   case 1:
      acmd->state = MONGOC_ASYNC_CMD_SEND;
      acmd->events = POLLOUT;
      break;
   default:
      abort ();
   }

   return MONGOC_ASYNC_CMD_IN_PROGRESS;
}

mongoc_async_cmd_result_t
_mongoc_async_cmd_phase_send (mongoc_async_cmd_t *acmd)
{
   ssize_t bytes;

   bytes = mongoc_stream_writev (acmd->stream, acmd->iovec, acmd->niovec, 0);

   if (bytes < 0) {
      bson_set_error (&acmd->error,
                      MONGOC_ERROR_STREAM,
                      MONGOC_ERROR_STREAM_SOCKET,
                      "Failed to write rpc bytes.");
      return MONGOC_ASYNC_CMD_ERROR;
   }

   while (bytes) {
      if (acmd->iovec->iov_len < (size_t) bytes) {
         bytes -= acmd->iovec->iov_len;
         acmd->iovec++;
         acmd->niovec--;
      } else {
         acmd->iovec->iov_base = ((char *) acmd->iovec->iov_base) + bytes;
         acmd->iovec->iov_len -= bytes;
         bytes = 0;
      }
   }

   acmd->state = MONGOC_ASYNC_CMD_RECV_LEN;
   acmd->bytes_to_read = 4;
   acmd->events = POLLIN;

   acmd->cmd_started = bson_get_monotonic_time ();

   return MONGOC_ASYNC_CMD_IN_PROGRESS;
}

mongoc_async_cmd_result_t
_mongoc_async_cmd_phase_recv_len (mongoc_async_cmd_t *acmd)
{
   ssize_t bytes = _mongoc_buffer_try_append_from_stream (
      &acmd->buffer, acmd->stream, acmd->bytes_to_read, 0);
   uint32_t msg_len;

   if (bytes < 0) {
      bson_set_error (&acmd->error,
                      MONGOC_ERROR_STREAM,
                      MONGOC_ERROR_STREAM_SOCKET,
                      "Failed to receive length header from server.");
      return MONGOC_ASYNC_CMD_ERROR;
   }

   if (bytes == 0) {
      bson_set_error (&acmd->error,
                      MONGOC_ERROR_STREAM,
                      MONGOC_ERROR_STREAM_SOCKET,
                      "Server closed connection.");
      return MONGOC_ASYNC_CMD_ERROR;
   }

   acmd->bytes_to_read -= bytes;

   if (!acmd->bytes_to_read) {
      memcpy (&msg_len, acmd->buffer.data, 4);
      msg_len = BSON_UINT32_FROM_LE (msg_len);

      if ((msg_len < 16) || (msg_len > MONGOC_DEFAULT_MAX_MSG_SIZE)) {
         bson_set_error (&acmd->error,
                         MONGOC_ERROR_PROTOCOL,
                         MONGOC_ERROR_PROTOCOL_INVALID_REPLY,
                         "Invalid reply from server.");
         return MONGOC_ASYNC_CMD_ERROR;
      }

      acmd->bytes_to_read = msg_len - 4;
      acmd->state = MONGOC_ASYNC_CMD_RECV_RPC;

      return _mongoc_async_cmd_phase_recv_rpc (acmd);
   }

   return MONGOC_ASYNC_CMD_IN_PROGRESS;
}

mongoc_async_cmd_result_t
_mongoc_async_cmd_phase_recv_rpc (mongoc_async_cmd_t *acmd)
{
   ssize_t bytes = _mongoc_buffer_try_append_from_stream (
      &acmd->buffer, acmd->stream, acmd->bytes_to_read, 0);

   if (bytes < 0) {
      bson_set_error (&acmd->error,
                      MONGOC_ERROR_STREAM,
                      MONGOC_ERROR_STREAM_SOCKET,
                      "Failed to receive rpc bytes from server.");
      return MONGOC_ASYNC_CMD_ERROR;
   }

   if (bytes == 0) {
      bson_set_error (&acmd->error,
                      MONGOC_ERROR_STREAM,
                      MONGOC_ERROR_STREAM_SOCKET,
                      "Server closed connection.");
      return MONGOC_ASYNC_CMD_ERROR;
   }

   acmd->bytes_to_read -= bytes;

   if (!acmd->bytes_to_read) {
      if (!_mongoc_rpc_scatter (
             &acmd->rpc, acmd->buffer.data, acmd->buffer.len)) {
         bson_set_error (&acmd->error,
                         MONGOC_ERROR_PROTOCOL,
                         MONGOC_ERROR_PROTOCOL_INVALID_REPLY,
                         "Invalid reply from server.");
         return MONGOC_ASYNC_CMD_ERROR;
      }
      if (BSON_UINT32_FROM_LE (acmd->rpc.header.opcode) ==
          MONGOC_OPCODE_COMPRESSED) {
         uint8_t *buf = NULL;
         size_t len =
            BSON_UINT32_FROM_LE (acmd->rpc.compressed.uncompressed_size) +
            sizeof (mongoc_rpc_header_t);

         buf = bson_malloc0 (len);
         if (!_mongoc_rpc_decompress (&acmd->rpc, buf, len)) {
            bson_free (buf);
            bson_set_error (&acmd->error,
                            MONGOC_ERROR_PROTOCOL,
                            MONGOC_ERROR_PROTOCOL_INVALID_REPLY,
                            "Could not decompress server reply");
            return MONGOC_ASYNC_CMD_ERROR;
         }

         _mongoc_buffer_destroy (&acmd->buffer);
         _mongoc_buffer_init (&acmd->buffer, buf, len, NULL, NULL);
      }

      _mongoc_rpc_swab_from_le (&acmd->rpc);

      if (!_mongoc_rpc_get_first_document (&acmd->rpc, &acmd->reply)) {
         bson_set_error (&acmd->error,
                         MONGOC_ERROR_PROTOCOL,
                         MONGOC_ERROR_PROTOCOL_INVALID_REPLY,
                         "Invalid reply from server");
         return MONGOC_ASYNC_CMD_ERROR;
      }

      acmd->reply_needs_cleanup = true;

      return MONGOC_ASYNC_CMD_SUCCESS;
   }

   return MONGOC_ASYNC_CMD_IN_PROGRESS;
}
