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


#include "mongoc-cursor.h"
#include "mongoc-cursor-private.h"
#include "mongoc-client-private.h"
#include "mongoc-counters-private.h"
#include "mongoc-error.h"
#include "mongoc-log.h"
#include "mongoc-trace-private.h"
#include "mongoc-cursor-cursorid-private.h"
#include "mongoc-read-concern-private.h"
#include "mongoc-util-private.h"
#include "mongoc-write-concern-private.h"


#undef MONGOC_LOG_DOMAIN
#define MONGOC_LOG_DOMAIN "cursor"


#define CURSOR_FAILED(cursor_) ((cursor_)->error.domain != 0)

static bool
_translate_query_opt (const char *query_field,
                      const char **cmd_field,
                      int *len);

static const bson_t *
_mongoc_cursor_op_query (mongoc_cursor_t *cursor,
                         mongoc_server_stream_t *server_stream);

static bool
_mongoc_cursor_prepare_find_command (mongoc_cursor_t *cursor,
                                     bson_t *command,
                                     mongoc_server_stream_t *server_stream);

static const bson_t *
_mongoc_cursor_find_command (mongoc_cursor_t *cursor,
                             mongoc_server_stream_t *server_stream);


static bool
_mongoc_cursor_set_opt_int64 (mongoc_cursor_t *cursor,
                              const char *option,
                              int64_t value)
{
   bson_iter_t iter;

   if (bson_iter_init_find (&iter, &cursor->opts, option)) {
      if (!BSON_ITER_HOLDS_INT64 (&iter)) {
         return false;
      }

      bson_iter_overwrite_int64 (&iter, value);
      return true;
   }

   return BSON_APPEND_INT64 (&cursor->opts, option, value);
}


static int64_t
_mongoc_cursor_get_opt_int64 (const mongoc_cursor_t *cursor,
                              const char *option,
                              int64_t default_value)
{
   bson_iter_t iter;

   if (bson_iter_init_find (&iter, &cursor->opts, option)) {
      return bson_iter_as_int64 (&iter);
   }

   return default_value;
}


static bool
_mongoc_cursor_set_opt_bool (mongoc_cursor_t *cursor,
                             const char *option,
                             bool value)
{
   bson_iter_t iter;

   if (bson_iter_init_find (&iter, &cursor->opts, option)) {
      if (!BSON_ITER_HOLDS_BOOL (&iter)) {
         return false;
      }

      bson_iter_overwrite_bool (&iter, value);
      return true;
   }

   return BSON_APPEND_BOOL (&cursor->opts, option, value);
}


bool
_mongoc_cursor_get_opt_bool (const mongoc_cursor_t *cursor, const char *option)
{
   bson_iter_t iter;

   if (bson_iter_init_find (&iter, &cursor->opts, option)) {
      return bson_iter_as_bool (&iter);
   }

   return false;
}


int32_t
_mongoc_n_return (mongoc_cursor_t *cursor)
{
   int64_t limit;
   int64_t batch_size;
   int64_t n_return;

   if (cursor->is_command) {
      /* commands always have n_return of 1 */
      return 1;
   }

   limit = mongoc_cursor_get_limit (cursor);
   batch_size = mongoc_cursor_get_batch_size (cursor);

   if (limit < 0) {
      n_return = limit;
   } else if (limit) {
      int64_t remaining = limit - cursor->count;
      BSON_ASSERT (remaining > 0);

      if (batch_size) {
         n_return = BSON_MIN (batch_size, remaining);
      } else {
         /* batch_size 0 means accept the default */
         n_return = remaining;
      }
   } else {
      n_return = batch_size;
   }

   if (n_return < INT32_MIN) {
      return INT32_MIN;
   } else if (n_return > INT32_MAX) {
      return INT32_MAX;
   } else {
      return (int32_t) n_return;
   }
}


void
_mongoc_set_cursor_ns (mongoc_cursor_t *cursor, const char *ns, uint32_t nslen)
{
   const char *dot;

   bson_strncpy (cursor->ns, ns, sizeof cursor->ns);
   cursor->nslen = BSON_MIN (nslen, sizeof cursor->ns);
   dot = strstr (cursor->ns, ".");

   if (dot) {
      cursor->dblen = (uint32_t) (dot - cursor->ns);
   } else {
      /* a database name with no collection name */
      cursor->dblen = cursor->nslen;
   }
}


/* return first key beginning with $, or NULL. precondition: bson is valid. */
static const char *
_first_dollar_field (const bson_t *bson)
{
   bson_iter_t iter;
   const char *key;

   BSON_ASSERT (bson_iter_init (&iter, bson));
   while (bson_iter_next (&iter)) {
      key = bson_iter_key (&iter);

      if (key[0] == '$') {
         return key;
      }
   }

   return NULL;
}


#define MARK_FAILED(c)          \
   do {                         \
      (c)->done = true;         \
      (c)->end_of_event = true; \
      (c)->sent = true;         \
   } while (0)


mongoc_cursor_t *
_mongoc_cursor_new_with_opts (mongoc_client_t *client,
                              const char *db_and_collection,
                              bool is_command,
                              const bson_t *filter,
                              const bson_t *opts,
                              const mongoc_read_prefs_t *read_prefs,
                              const mongoc_read_concern_t *read_concern)
{
   mongoc_cursor_t *cursor;
   mongoc_topology_description_type_t td_type;
   uint32_t server_id;
   bson_error_t validate_err;
   const char *dollar_field;

   ENTRY;

   BSON_ASSERT (client);

   cursor = (mongoc_cursor_t *) bson_malloc0 (sizeof *cursor);
   cursor->client = client;
   cursor->is_command = is_command ? 1 : 0;

   bson_init (&cursor->filter);
   bson_init (&cursor->opts);
   bson_init (&cursor->error_doc);

   if (filter) {
      if (!bson_validate_with_error (
             filter, BSON_VALIDATE_EMPTY_KEYS, &validate_err)) {
         MARK_FAILED (cursor);
         bson_set_error (&cursor->error,
                         MONGOC_ERROR_CURSOR,
                         MONGOC_ERROR_CURSOR_INVALID_CURSOR,
                         "Invalid filter: %s",
                         validate_err.message);
         GOTO (finish);
      }

      bson_destroy (&cursor->filter);
      bson_copy_to (filter, &cursor->filter);
   }

   if (opts) {
      if (!bson_validate_with_error (
             opts, BSON_VALIDATE_EMPTY_KEYS, &validate_err)) {
         MARK_FAILED (cursor);
         bson_set_error (&cursor->error,
                         MONGOC_ERROR_CURSOR,
                         MONGOC_ERROR_CURSOR_INVALID_CURSOR,
                         "Invalid opts: %s",
                         validate_err.message);
         GOTO (finish);
      }

      dollar_field = _first_dollar_field (opts);
      if (dollar_field) {
         MARK_FAILED (cursor);
         bson_set_error (&cursor->error,
                         MONGOC_ERROR_CURSOR,
                         MONGOC_ERROR_CURSOR_INVALID_CURSOR,
                         "Cannot use $-modifiers in opts: \"%s\"",
                         dollar_field);
         GOTO (finish);
      }

      bson_copy_to_excluding_noinit (opts, &cursor->opts, "serverId", NULL);

      /* true if there's a valid serverId or no serverId, false on err */
      if (!_mongoc_get_server_id_from_opts (opts,
                                            MONGOC_ERROR_CURSOR,
                                            MONGOC_ERROR_CURSOR_INVALID_CURSOR,
                                            &server_id,
                                            &cursor->error)) {
         MARK_FAILED (cursor);
         GOTO (finish);
      }

      if (server_id) {
         mongoc_cursor_set_hint (cursor, server_id);
      }
   }

   cursor->read_prefs = read_prefs
                           ? mongoc_read_prefs_copy (read_prefs)
                           : mongoc_read_prefs_new (MONGOC_READ_PRIMARY);

   cursor->read_concern = read_concern ? mongoc_read_concern_copy (read_concern)
                                       : mongoc_read_concern_new ();

   if (db_and_collection) {
      _mongoc_set_cursor_ns (
         cursor, db_and_collection, (uint32_t) strlen (db_and_collection));
   }

   if (_mongoc_cursor_get_opt_bool (cursor, MONGOC_CURSOR_EXHAUST)) {
      if (_mongoc_cursor_get_opt_int64 (cursor, MONGOC_CURSOR_LIMIT, 0)) {
         bson_set_error (&cursor->error,
                         MONGOC_ERROR_CURSOR,
                         MONGOC_ERROR_CURSOR_INVALID_CURSOR,
                         "Cannot specify both 'exhaust' and 'limit'.");
         MARK_FAILED (cursor);
         GOTO (finish);
      }

      td_type = _mongoc_topology_get_type (client->topology);

      if (td_type == MONGOC_TOPOLOGY_SHARDED) {
         bson_set_error (&cursor->error,
                         MONGOC_ERROR_CURSOR,
                         MONGOC_ERROR_CURSOR_INVALID_CURSOR,
                         "Cannot use exhaust cursor with sharded cluster.");
         MARK_FAILED (cursor);
         GOTO (finish);
      }
   }

   _mongoc_buffer_init (&cursor->buffer, NULL, 0, NULL, NULL);
   _mongoc_read_prefs_validate (read_prefs, &cursor->error);

finish:
   mongoc_counter_cursors_active_inc ();

   RETURN (cursor);
}


mongoc_cursor_t *
_mongoc_cursor_new (mongoc_client_t *client,
                    const char *db_and_collection,
                    mongoc_query_flags_t qflags,
                    uint32_t skip,
                    int32_t limit,
                    uint32_t batch_size,
                    bool is_command,
                    const bson_t *query,
                    const bson_t *fields,
                    const mongoc_read_prefs_t *read_prefs,
                    const mongoc_read_concern_t *read_concern)
{
   bson_t filter;
   bool has_filter = false;
   bson_t opts = BSON_INITIALIZER;
   bool slave_ok = false;
   const char *key;
   bson_iter_t iter;
   const char *opt_key;
   int len;
   uint32_t data_len;
   const uint8_t *data;
   mongoc_cursor_t *cursor;
   bson_error_t error = {0};

   ENTRY;

   BSON_ASSERT (client);

   if (query) {
      if (bson_has_field (query, "$query")) {
         /* like "{$query: {a: 1}, $orderby: {b: 1}, $otherModifier: true}" */
         bson_iter_init (&iter, query);
         while (bson_iter_next (&iter)) {
            key = bson_iter_key (&iter);

            if (key[0] != '$') {
               bson_set_error (&error,
                               MONGOC_ERROR_CURSOR,
                               MONGOC_ERROR_CURSOR_INVALID_CURSOR,
                               "Cannot mix $query with non-dollar field '%s'",
                               key);
               GOTO (done);
            }

            if (!strcmp (key, "$query")) {
               /* set "filter" to the incoming document's "$query" */
               bson_iter_document (&iter, &data_len, &data);
               bson_init_static (&filter, data, (size_t) data_len);
               has_filter = true;
            } else if (_translate_query_opt (key, &opt_key, &len)) {
               /* "$orderby" becomes "sort", etc., "$unknown" -> "unknown" */
               bson_append_iter (&opts, opt_key, len, &iter);
            } else {
               /* strip leading "$" */
               bson_append_iter (&opts, key + 1, -1, &iter);
            }
         }
      }
   }

   if (!bson_empty0 (fields)) {
      bson_append_document (
         &opts, MONGOC_CURSOR_PROJECTION, MONGOC_CURSOR_PROJECTION_LEN, fields);
   }

   if (skip) {
      bson_append_int64 (
         &opts, MONGOC_CURSOR_SKIP, MONGOC_CURSOR_SKIP_LEN, skip);
   }

   if (limit) {
      bson_append_int64 (
         &opts, MONGOC_CURSOR_LIMIT, MONGOC_CURSOR_LIMIT_LEN, llabs (limit));

      if (limit < 0) {
         bson_append_bool (&opts,
                           MONGOC_CURSOR_SINGLE_BATCH,
                           MONGOC_CURSOR_SINGLE_BATCH_LEN,
                           true);
      }
   }

   if (batch_size) {
      bson_append_int64 (&opts,
                         MONGOC_CURSOR_BATCH_SIZE,
                         MONGOC_CURSOR_BATCH_SIZE_LEN,
                         batch_size);
   }

   if (qflags & MONGOC_QUERY_SLAVE_OK) {
      slave_ok = true;
   }

   if (qflags & MONGOC_QUERY_TAILABLE_CURSOR) {
      bson_append_bool (
         &opts, MONGOC_CURSOR_TAILABLE, MONGOC_CURSOR_TAILABLE_LEN, true);
   }

   if (qflags & MONGOC_QUERY_OPLOG_REPLAY) {
      bson_append_bool (&opts,
                        MONGOC_CURSOR_OPLOG_REPLAY,
                        MONGOC_CURSOR_OPLOG_REPLAY_LEN,
                        true);
   }

   if (qflags & MONGOC_QUERY_NO_CURSOR_TIMEOUT) {
      bson_append_bool (&opts,
                        MONGOC_CURSOR_NO_CURSOR_TIMEOUT,
                        MONGOC_CURSOR_NO_CURSOR_TIMEOUT_LEN,
                        true);
   }

   if (qflags & MONGOC_QUERY_AWAIT_DATA) {
      bson_append_bool (
         &opts, MONGOC_CURSOR_AWAIT_DATA, MONGOC_CURSOR_AWAIT_DATA_LEN, true);
   }

   if (qflags & MONGOC_QUERY_EXHAUST) {
      bson_append_bool (
         &opts, MONGOC_CURSOR_EXHAUST, MONGOC_CURSOR_EXHAUST_LEN, true);
   }

   if (qflags & MONGOC_QUERY_PARTIAL) {
      bson_append_bool (&opts,
                        MONGOC_CURSOR_ALLOW_PARTIAL_RESULTS,
                        MONGOC_CURSOR_ALLOW_PARTIAL_RESULTS_LEN,
                        true);
   }

done:

   if (error.domain != 0) {
      cursor = _mongoc_cursor_new_with_opts (
         client, db_and_collection, is_command, NULL, NULL, NULL, NULL);

      MARK_FAILED (cursor);
      memcpy (&cursor->error, &error, sizeof (bson_error_t));
   } else {
      cursor = _mongoc_cursor_new_with_opts (client,
                                             db_and_collection,
                                             is_command,
                                             has_filter ? &filter : query,
                                             &opts,
                                             read_prefs,
                                             read_concern);

      if (slave_ok) {
         cursor->slave_ok = true;
      }
   }

   if (has_filter) {
      bson_destroy (&filter);
   }

   bson_destroy (&opts);

   RETURN (cursor);
}


void
mongoc_cursor_destroy (mongoc_cursor_t *cursor)
{
   ENTRY;

   BSON_ASSERT (cursor);

   if (cursor->iface.destroy) {
      cursor->iface.destroy (cursor);
   } else {
      _mongoc_cursor_destroy (cursor);
   }

   EXIT;
}

void
_mongoc_cursor_destroy (mongoc_cursor_t *cursor)
{
   char db[MONGOC_NAMESPACE_MAX];
   ENTRY;

   BSON_ASSERT (cursor);

   if (cursor->in_exhaust) {
      cursor->client->in_exhaust = false;
      if (!cursor->done) {
         /* The only way to stop an exhaust cursor is to kill the connection */
         mongoc_cluster_disconnect_node (&cursor->client->cluster,
                                         cursor->server_id);
      }
   } else if (cursor->rpc.reply.cursor_id) {
      bson_strncpy (db, cursor->ns, cursor->dblen + 1);

      _mongoc_client_kill_cursor (cursor->client,
                                  cursor->server_id,
                                  cursor->rpc.reply.cursor_id,
                                  cursor->operation_id,
                                  db,
                                  cursor->ns + cursor->dblen + 1);
   }

   if (cursor->reader) {
      bson_reader_destroy (cursor->reader);
      cursor->reader = NULL;
   }

   _mongoc_buffer_destroy (&cursor->buffer);
   mongoc_read_prefs_destroy (cursor->read_prefs);
   mongoc_read_concern_destroy (cursor->read_concern);
   mongoc_write_concern_destroy (cursor->write_concern);

   bson_destroy (&cursor->filter);
   bson_destroy (&cursor->opts);
   bson_destroy (&cursor->error_doc);
   bson_free (cursor);

   mongoc_counter_cursors_active_dec ();
   mongoc_counter_cursors_disposed_inc ();

   EXIT;
}


mongoc_server_stream_t *
_mongoc_cursor_fetch_stream (mongoc_cursor_t *cursor)
{
   mongoc_server_stream_t *server_stream;

   ENTRY;

   if (cursor->server_id) {
      server_stream =
         mongoc_cluster_stream_for_server (&cursor->client->cluster,
                                           cursor->server_id,
                                           true /* reconnect_ok */,
                                           &cursor->error);
   } else {
      server_stream = mongoc_cluster_stream_for_reads (
         &cursor->client->cluster, cursor->read_prefs, &cursor->error);

      if (server_stream) {
         cursor->server_id = server_stream->sd->id;
      }
   }

   RETURN (server_stream);
}


bool
_use_find_command (const mongoc_cursor_t *cursor,
                   const mongoc_server_stream_t *server_stream)
{
   /* Find, getMore And killCursors Commands Spec: "the find command cannot be
    * used to execute other commands" and "the find command does not support the
    * exhaust flag."
    */
   return server_stream->sd->max_wire_version >= WIRE_VERSION_FIND_CMD &&
          !cursor->is_command &&
          !_mongoc_cursor_get_opt_bool (cursor, MONGOC_CURSOR_EXHAUST);
}


bool
_use_getmore_command (const mongoc_cursor_t *cursor,
                      const mongoc_server_stream_t *server_stream)
{
   return server_stream->sd->max_wire_version >= WIRE_VERSION_FIND_CMD &&
          !_mongoc_cursor_get_opt_bool (cursor, MONGOC_CURSOR_EXHAUST);
}


static const bson_t *
_mongoc_cursor_initial_query (mongoc_cursor_t *cursor)
{
   mongoc_server_stream_t *server_stream;
   const bson_t *b = NULL;

   ENTRY;

   BSON_ASSERT (cursor);

   server_stream = _mongoc_cursor_fetch_stream (cursor);

   if (!server_stream) {
      GOTO (done);
   }

   if (_use_find_command (cursor, server_stream)) {
      b = _mongoc_cursor_find_command (cursor, server_stream);
   } else {
      /* When the user explicitly provides a readConcern -- but the server
       * doesn't support readConcern, we must error:
       * https://github.com/mongodb/specifications/blob/master/source/read-write-concern/read-write-concern.rst#errors-1
       */
      if (cursor->read_concern->level != NULL &&
          server_stream->sd->max_wire_version < WIRE_VERSION_READ_CONCERN) {
         bson_set_error (&cursor->error,
                         MONGOC_ERROR_COMMAND,
                         MONGOC_ERROR_PROTOCOL_BAD_WIRE_VERSION,
                         "The selected server does not support readConcern");
      } else {
         b = _mongoc_cursor_op_query (cursor, server_stream);
      }
   }

done:
   /* no-op if server_stream is NULL */
   mongoc_server_stream_cleanup (server_stream);

   if (!b) {
      cursor->done = true;
   }

   RETURN (b);
}


static bool
_mongoc_cursor_monitor_legacy_query (mongoc_cursor_t *cursor,
                                     mongoc_server_stream_t *server_stream,
                                     const char *cmd_name)
{
   bson_t doc;
   mongoc_client_t *client;
   mongoc_apm_command_started_t event;
   char db[MONGOC_NAMESPACE_MAX];

   ENTRY;

   client = cursor->client;
   if (!client->apm_callbacks.started) {
      /* successful */
      RETURN (true);
   }

   bson_init (&doc);
   bson_strncpy (db, cursor->ns, cursor->dblen + 1);

   if (!cursor->is_command) {
      /* simulate a MongoDB 3.2+ "find" command */
      if (!_mongoc_cursor_prepare_find_command (cursor, &doc, server_stream)) {
         /* cursor->error is set */
         bson_destroy (&doc);
         RETURN (false);
      }
   }

   mongoc_apm_command_started_init (&event,
                                    cursor->is_command ? &cursor->filter : &doc,
                                    db,
                                    cmd_name,
                                    client->cluster.request_id,
                                    cursor->operation_id,
                                    &server_stream->sd->host,
                                    server_stream->sd->id,
                                    client->apm_context);

   client->apm_callbacks.started (&event);
   mongoc_apm_command_started_cleanup (&event);
   bson_destroy (&doc);

   RETURN (true);
}


/* append array of docs from current cursor batch */
static void
_mongoc_cursor_append_docs_array (mongoc_cursor_t *cursor, bson_t *docs)
{
   bool eof = false;
   char str[16];
   const char *key;
   uint32_t i = 0;
   size_t keylen;
   const bson_t *doc;

   while ((doc = bson_reader_read (cursor->reader, &eof))) {
      keylen = bson_uint32_to_string (i, &key, str, sizeof str);
      bson_append_document (docs, key, (int) keylen, doc);
   }

   bson_reader_reset (cursor->reader);
}


static void
_mongoc_cursor_monitor_succeeded (mongoc_cursor_t *cursor,
                                  int64_t duration,
                                  bool first_batch,
                                  mongoc_server_stream_t *stream,
                                  const char *cmd_name)
{
   mongoc_apm_command_succeeded_t event;
   mongoc_client_t *client;
   bson_t reply;
   bson_t reply_cursor;

   ENTRY;

   client = cursor->client;

   if (!client->apm_callbacks.succeeded) {
      EXIT;
   }

   if (cursor->is_command) {
      /* cursor is from mongoc_client_command. we're in mongoc_cursor_next. */
      if (!_mongoc_rpc_reply_get_first (&cursor->rpc.reply, &reply)) {
         MONGOC_ERROR ("_mongoc_cursor_monitor_succeeded can't parse reply");
         EXIT;
      }
   } else {
      bson_t docs_array;

      /* fake reply to find/getMore command:
       * {ok: 1, cursor: {id: 17, ns: "...", first/nextBatch: [ ... docs ... ]}}
       */
      bson_init (&docs_array);
      _mongoc_cursor_append_docs_array (cursor, &docs_array);

      bson_init (&reply);
      bson_append_int32 (&reply, "ok", 2, 1);
      bson_append_document_begin (&reply, "cursor", 6, &reply_cursor);
      bson_append_int64 (&reply_cursor, "id", 2, mongoc_cursor_get_id (cursor));
      bson_append_utf8 (&reply_cursor, "ns", 2, cursor->ns, cursor->nslen);
      bson_append_array (&reply_cursor,
                         first_batch ? "firstBatch" : "nextBatch",
                         first_batch ? 10 : 9,
                         &docs_array);
      bson_append_document_end (&reply, &reply_cursor);
      bson_destroy (&docs_array);
   }

   mongoc_apm_command_succeeded_init (&event,
                                      duration,
                                      &reply,
                                      cmd_name,
                                      client->cluster.request_id,
                                      cursor->operation_id,
                                      &stream->sd->host,
                                      stream->sd->id,
                                      client->apm_context);

   client->apm_callbacks.succeeded (&event);

   mongoc_apm_command_succeeded_cleanup (&event);
   bson_destroy (&reply);

   EXIT;
}


static void
_mongoc_cursor_monitor_failed (mongoc_cursor_t *cursor,
                               int64_t duration,
                               mongoc_server_stream_t *stream,
                               const char *cmd_name)
{
   mongoc_apm_command_failed_t event;
   mongoc_client_t *client;

   ENTRY;

   client = cursor->client;

   if (!client->apm_callbacks.failed) {
      EXIT;
   }

   mongoc_apm_command_failed_init (&event,
                                   duration,
                                   cmd_name,
                                   &cursor->error,
                                   client->cluster.request_id,
                                   cursor->operation_id,
                                   &stream->sd->host,
                                   stream->sd->id,
                                   client->apm_context);

   client->apm_callbacks.failed (&event);

   mongoc_apm_command_failed_cleanup (&event);

   EXIT;
}


#define OPT_CHECK(_type)                                         \
   do {                                                          \
      if (!BSON_ITER_HOLDS_##_type (&iter)) {                    \
         bson_set_error (&cursor->error,                         \
                         MONGOC_ERROR_COMMAND,                   \
                         MONGOC_ERROR_COMMAND_INVALID_ARG,       \
                         "invalid option %s, should be type %s", \
                         key,                                    \
                         #_type);                                \
         return NULL;                                            \
      }                                                          \
   } while (false)


#define OPT_CHECK_INT()                                          \
   do {                                                          \
      if (!BSON_ITER_HOLDS_INT (&iter)) {                        \
         bson_set_error (&cursor->error,                         \
                         MONGOC_ERROR_COMMAND,                   \
                         MONGOC_ERROR_COMMAND_INVALID_ARG,       \
                         "invalid option %s, should be integer", \
                         key);                                   \
         return NULL;                                            \
      }                                                          \
   } while (false)


#define OPT_ERR(_msg)                                   \
   do {                                                 \
      bson_set_error (&cursor->error,                   \
                      MONGOC_ERROR_COMMAND,             \
                      MONGOC_ERROR_COMMAND_INVALID_ARG, \
                      _msg);                            \
      return NULL;                                      \
   } while (false)


#define OPT_BSON_ERR(_msg)                                                    \
   do {                                                                       \
      bson_set_error (                                                        \
         &cursor->error, MONGOC_ERROR_BSON, MONGOC_ERROR_BSON_INVALID, _msg); \
      return NULL;                                                            \
   } while (false)


#define OPT_FLAG(_flag)                \
   do {                                \
      OPT_CHECK (BOOL);                \
      if (bson_iter_as_bool (&iter)) { \
         *flags |= _flag;              \
      }                                \
   } while (false)


#define PUSH_DOLLAR_QUERY()                                          \
   do {                                                              \
      if (!pushed_dollar_query) {                                    \
         pushed_dollar_query = true;                                 \
         bson_append_document (query, "$query", 6, &cursor->filter); \
      }                                                              \
   } while (false)


#define OPT_SUBDOCUMENT(_opt_name, _legacy_name)                           \
   do {                                                                    \
      OPT_CHECK (DOCUMENT);                                                \
      bson_iter_document (&iter, &len, &data);                             \
      if (!bson_init_static (&subdocument, data, (size_t) len)) {          \
         OPT_BSON_ERR ("Invalid '" #_opt_name "' subdocument in 'opts'."); \
      }                                                                    \
      BSON_APPEND_DOCUMENT (query, "$" #_legacy_name, &subdocument);       \
   } while (false)

#define ADD_FLAG(_flags, _value)                                   \
   do {                                                            \
      if (!BSON_ITER_HOLDS_BOOL (&iter)) {                         \
         bson_set_error (&cursor->error,                           \
                         MONGOC_ERROR_COMMAND,                     \
                         MONGOC_ERROR_COMMAND_INVALID_ARG,         \
                         "invalid option %s, should be type bool", \
                         key);                                     \
         return false;                                             \
      }                                                            \
      if (bson_iter_as_bool (&iter)) {                             \
         *_flags |= _value;                                        \
      }                                                            \
   } while (false);

static bool
_mongoc_cursor_flags (mongoc_cursor_t *cursor,
                      mongoc_server_stream_t *stream,
                      mongoc_query_flags_t *flags /* OUT */)
{
   bson_iter_t iter;
   const char *key;

   *flags = MONGOC_QUERY_NONE;

   if (!bson_iter_init (&iter, &cursor->opts)) {
      bson_set_error (&cursor->error,
                      MONGOC_ERROR_BSON,
                      MONGOC_ERROR_BSON_INVALID,
                      "Invalid 'opts' parameter.");
      return false;
   }

   while (bson_iter_next (&iter)) {
      key = bson_iter_key (&iter);

      if (!strcmp (key, MONGOC_CURSOR_ALLOW_PARTIAL_RESULTS)) {
         ADD_FLAG (flags, MONGOC_QUERY_PARTIAL);
      } else if (!strcmp (key, MONGOC_CURSOR_AWAIT_DATA)) {
         ADD_FLAG (flags, MONGOC_QUERY_AWAIT_DATA);
      } else if (!strcmp (key, MONGOC_CURSOR_EXHAUST)) {
         ADD_FLAG (flags, MONGOC_QUERY_EXHAUST);
      } else if (!strcmp (key, MONGOC_CURSOR_NO_CURSOR_TIMEOUT)) {
         ADD_FLAG (flags, MONGOC_QUERY_NO_CURSOR_TIMEOUT);
      } else if (!strcmp (key, MONGOC_CURSOR_OPLOG_REPLAY)) {
         ADD_FLAG (flags, MONGOC_QUERY_OPLOG_REPLAY);
      } else if (!strcmp (key, MONGOC_CURSOR_TAILABLE)) {
         ADD_FLAG (flags, MONGOC_QUERY_TAILABLE_CURSOR);
      }
   }

   if (cursor->slave_ok) {
      *flags |= MONGOC_QUERY_SLAVE_OK;
   } else if (cursor->server_id_set &&
              (stream->topology_type == MONGOC_TOPOLOGY_RS_WITH_PRIMARY ||
               stream->topology_type == MONGOC_TOPOLOGY_RS_NO_PRIMARY) &&
              stream->sd->type != MONGOC_SERVER_RS_PRIMARY) {
      *flags |= MONGOC_QUERY_SLAVE_OK;
   }

   return true;
}


static bson_t *
_mongoc_cursor_parse_opts_for_op_query (mongoc_cursor_t *cursor,
                                        mongoc_server_stream_t *stream,
                                        bson_t *query /* OUT */,
                                        bson_t *fields /* OUT */,
                                        mongoc_query_flags_t *flags /* OUT */,
                                        int32_t *skip /* OUT */)
{
   bool pushed_dollar_query;
   bson_iter_t iter;
   uint32_t len;
   const uint8_t *data;
   bson_t subdocument;
   const char *key;
   char *dollar_modifier;

   *flags = MONGOC_QUERY_NONE;
   *skip = 0;

   /* assume we'll send filter straight to server, like "{a: 1}". if we find an
    * opt we must add, like "sort", we push the query like "$query: {a: 1}",
    * then add a query modifier for the option, in this example "$orderby".
    */
   pushed_dollar_query = false;

   if (!bson_iter_init (&iter, &cursor->opts)) {
      OPT_BSON_ERR ("Invalid 'opts' parameter.");
   }

   while (bson_iter_next (&iter)) {
      key = bson_iter_key (&iter);

      /* most common options first */
      if (!strcmp (key, MONGOC_CURSOR_PROJECTION)) {
         OPT_CHECK (DOCUMENT);
         bson_iter_document (&iter, &len, &data);
         if (!bson_init_static (&subdocument, data, (size_t) len)) {
            OPT_BSON_ERR ("Invalid 'projection' subdocument in 'opts'.");
         }

         bson_copy_to (&subdocument, fields);
      } else if (!strcmp (key, MONGOC_CURSOR_SORT)) {
         PUSH_DOLLAR_QUERY ();
         OPT_SUBDOCUMENT (sort, orderby);
      } else if (!strcmp (key, MONGOC_CURSOR_SKIP)) {
         OPT_CHECK_INT ();
         *skip = (int32_t) bson_iter_as_int64 (&iter);
      }
      /* the rest of the options, alphabetically */
      else if (!strcmp (key, MONGOC_CURSOR_ALLOW_PARTIAL_RESULTS)) {
         OPT_FLAG (MONGOC_QUERY_PARTIAL);
      } else if (!strcmp (key, MONGOC_CURSOR_AWAIT_DATA)) {
         OPT_FLAG (MONGOC_QUERY_AWAIT_DATA);
      } else if (!strcmp (key, MONGOC_CURSOR_COMMENT)) {
         OPT_CHECK (UTF8);
         PUSH_DOLLAR_QUERY ();
         BSON_APPEND_UTF8 (query, "$comment", bson_iter_utf8 (&iter, NULL));
      } else if (!strcmp (key, MONGOC_CURSOR_HINT)) {
         if (BSON_ITER_HOLDS_UTF8 (&iter)) {
            PUSH_DOLLAR_QUERY ();
            BSON_APPEND_UTF8 (query, "$hint", bson_iter_utf8 (&iter, NULL));
         } else if (BSON_ITER_HOLDS_DOCUMENT (&iter)) {
            PUSH_DOLLAR_QUERY ();
            OPT_SUBDOCUMENT (hint, hint);
         } else {
            OPT_ERR ("Wrong type for 'hint' field in 'opts'.");
         }
      } else if (!strcmp (key, MONGOC_CURSOR_MAX)) {
         PUSH_DOLLAR_QUERY ();
         OPT_SUBDOCUMENT (max, max);
      } else if (!strcmp (key, MONGOC_CURSOR_MAX_SCAN)) {
         OPT_CHECK_INT ();
         PUSH_DOLLAR_QUERY ();
         BSON_APPEND_INT64 (query, "$maxScan", bson_iter_as_int64 (&iter));
      } else if (!strcmp (key, MONGOC_CURSOR_MAX_TIME_MS)) {
         OPT_CHECK_INT ();
         PUSH_DOLLAR_QUERY ();
         BSON_APPEND_INT64 (query, "$maxTimeMS", bson_iter_as_int64 (&iter));
      } else if (!strcmp (key, MONGOC_CURSOR_MIN)) {
         PUSH_DOLLAR_QUERY ();
         OPT_SUBDOCUMENT (min, min);
      } else if (!strcmp (key, MONGOC_CURSOR_READ_CONCERN)) {
         OPT_ERR ("Set readConcern on client, database, or collection,"
                  " not in a query.");
      } else if (!strcmp (key, MONGOC_CURSOR_RETURN_KEY)) {
         OPT_CHECK (BOOL);
         PUSH_DOLLAR_QUERY ();
         BSON_APPEND_BOOL (query, "$returnKey", bson_iter_as_bool (&iter));
      } else if (!strcmp (key, MONGOC_CURSOR_SHOW_RECORD_ID)) {
         OPT_CHECK (BOOL);
         PUSH_DOLLAR_QUERY ();
         BSON_APPEND_BOOL (query, "$showDiskLoc", bson_iter_as_bool (&iter));
      } else if (!strcmp (key, MONGOC_CURSOR_SNAPSHOT)) {
         OPT_CHECK (BOOL);
         PUSH_DOLLAR_QUERY ();
         BSON_APPEND_BOOL (query, "$snapshot", bson_iter_as_bool (&iter));
      } else if (!strcmp (key, MONGOC_CURSOR_COLLATION)) {
         bson_set_error (&cursor->error,
                         MONGOC_ERROR_CURSOR,
                         MONGOC_ERROR_PROTOCOL_BAD_WIRE_VERSION,
                         "Collation is not supported by this server");
         return NULL;
      }
      /* singleBatch limit and batchSize are handled in _mongoc_n_return,
       * exhaust noCursorTimeout oplogReplay tailable in _mongoc_cursor_flags
       * maxAwaitTimeMS is handled in _mongoc_cursor_prepare_getmore_command
       */
      else if (strcmp (key, MONGOC_CURSOR_SINGLE_BATCH) &&
               strcmp (key, MONGOC_CURSOR_LIMIT) &&
               strcmp (key, MONGOC_CURSOR_BATCH_SIZE) &&
               strcmp (key, MONGOC_CURSOR_EXHAUST) &&
               strcmp (key, MONGOC_CURSOR_NO_CURSOR_TIMEOUT) &&
               strcmp (key, MONGOC_CURSOR_OPLOG_REPLAY) &&
               strcmp (key, MONGOC_CURSOR_TAILABLE) &&
               strcmp (key, MONGOC_CURSOR_MAX_AWAIT_TIME_MS)) {
         /* pass unrecognized options to server, prefixed with $ */
         PUSH_DOLLAR_QUERY ();
         dollar_modifier = bson_strdup_printf ("$%s", key);
         bson_append_iter (query, dollar_modifier, -1, &iter);
         bson_free (dollar_modifier);
      }
   }

   if (!_mongoc_cursor_flags (cursor, stream, flags)) {
      /* cursor->error is set */
      return NULL;
   }

   return pushed_dollar_query ? query : &cursor->filter;
}


#undef OPT_CHECK
#undef OPT_ERR
#undef OPT_BSON_ERR
#undef OPT_FLAG
#undef OPT_SUBDOCUMENT


static const bson_t *
_mongoc_cursor_op_query (mongoc_cursor_t *cursor,
                         mongoc_server_stream_t *server_stream)
{
   int64_t started;
   uint32_t request_id;
   mongoc_rpc_t rpc;
   const char *cmd_name; /* for command monitoring */
   const bson_t *query_ptr;
   bson_t query = BSON_INITIALIZER;
   bson_t fields = BSON_INITIALIZER;
   mongoc_query_flags_t flags;
   mongoc_apply_read_prefs_result_t result = READ_PREFS_RESULT_INIT;
   const bson_t *ret = NULL;
   bool succeeded = false;

   ENTRY;

   started = bson_get_monotonic_time ();

   cursor->sent = true;
   cursor->operation_id = ++cursor->client->cluster.operation_id;

   request_id = ++cursor->client->cluster.request_id;

   rpc.header.msg_len = 0;
   rpc.header.request_id = request_id;
   rpc.header.response_to = 0;
   rpc.header.opcode = MONGOC_OPCODE_QUERY;
   rpc.query.flags = MONGOC_QUERY_NONE;
   rpc.query.collection = cursor->ns;
   rpc.query.skip = 0;
   rpc.query.n_return = 0;
   rpc.query.fields = NULL;

   if (cursor->is_command) {
      /* "filter" isn't a query, it's like {commandName: ... }*/
      cmd_name = _mongoc_get_command_name (&cursor->filter);
      BSON_ASSERT (cmd_name);
   } else {
      cmd_name = "find";
   }

   query_ptr = _mongoc_cursor_parse_opts_for_op_query (
      cursor, server_stream, &query, &fields, &flags, &rpc.query.skip);

   if (!query_ptr) {
      /* invalid opts. cursor->error is set */
      GOTO (done);
   }

   apply_read_preferences (
      cursor->read_prefs, server_stream, query_ptr, flags, &result);

   rpc.query.query = bson_get_data (result.query_with_read_prefs);
   rpc.query.flags = result.flags;
   rpc.query.n_return = _mongoc_n_return (cursor);
   if (!bson_empty (&fields)) {
      rpc.query.fields = bson_get_data (&fields);
   }

   if (!_mongoc_cursor_monitor_legacy_query (cursor, server_stream, cmd_name)) {
      GOTO (done);
   }

   if (!mongoc_cluster_sendv_to_server (&cursor->client->cluster,
                                        &rpc,
                                        server_stream,
                                        NULL,
                                        &cursor->error)) {
      GOTO (done);
   }

   _mongoc_buffer_clear (&cursor->buffer, false);

   if (!_mongoc_client_recv (cursor->client,
                             &cursor->rpc,
                             &cursor->buffer,
                             server_stream,
                             &cursor->error)) {
      GOTO (done);
   }

   if (cursor->rpc.header.opcode != MONGOC_OPCODE_REPLY) {
      bson_set_error (&cursor->error,
                      MONGOC_ERROR_PROTOCOL,
                      MONGOC_ERROR_PROTOCOL_INVALID_REPLY,
                      "Invalid opcode. Expected %d, got %d.",
                      MONGOC_OPCODE_REPLY,
                      cursor->rpc.header.opcode);
      GOTO (done);
   }

   if (cursor->rpc.header.response_to != request_id) {
      bson_set_error (&cursor->error,
                      MONGOC_ERROR_PROTOCOL,
                      MONGOC_ERROR_PROTOCOL_INVALID_REPLY,
                      "Invalid response_to for query. Expected %d, got %d.",
                      request_id,
                      cursor->rpc.header.response_to);
      GOTO (done);
   }

   if (!_mongoc_rpc_check_ok (&cursor->rpc,
                              (bool) cursor->is_command,
                              cursor->client->error_api_version,
                              &cursor->error,
                              &cursor->error_doc)) {
      GOTO (done);
   }

   if (cursor->reader) {
      bson_reader_destroy (cursor->reader);
   }

   cursor->reader = bson_reader_new_from_data (
      cursor->rpc.reply.documents, (size_t) cursor->rpc.reply.documents_len);

   if (_mongoc_cursor_get_opt_bool (cursor, MONGOC_CURSOR_EXHAUST)) {
      cursor->in_exhaust = true;
      cursor->client->in_exhaust = true;
   }

   _mongoc_cursor_monitor_succeeded (cursor,
                                     bson_get_monotonic_time () - started,
                                     true, /* first_batch */
                                     server_stream,
                                     cmd_name);

   cursor->done = false;
   cursor->end_of_event = false;
   succeeded = true;

   _mongoc_read_from_buffer (cursor, &ret);

done:
   if (!succeeded) {
      _mongoc_cursor_monitor_failed (
         cursor, bson_get_monotonic_time () - started, server_stream, cmd_name);
   }

   apply_read_prefs_result_cleanup (&result);
   bson_destroy (&query);
   bson_destroy (&fields);

   if (!ret) {
      cursor->done = true;
   }

   RETURN (ret);
}


bool
_mongoc_cursor_run_command (mongoc_cursor_t *cursor,
                            const bson_t *command,
                            bson_t *reply)
{
   mongoc_cluster_t *cluster;
   mongoc_server_stream_t *server_stream;
   mongoc_cmd_parts_t parts;
   char db[MONGOC_NAMESPACE_MAX];
   bool ret = false;

   ENTRY;

   cluster = &cursor->client->cluster;
   mongoc_cmd_parts_init (&parts, db, MONGOC_QUERY_NONE, command);
   parts.read_prefs = cursor->read_prefs;
   parts.assembled.operation_id = cursor->operation_id;
   server_stream = _mongoc_cursor_fetch_stream (cursor);

   if (!server_stream) {
      GOTO (done);
   }

   bson_strncpy (db, cursor->ns, cursor->dblen + 1);
   parts.assembled.db_name = db;

   if (!_mongoc_cursor_flags (cursor, server_stream, &parts.user_query_flags)) {
      GOTO (done);
   }

   if (cursor->write_concern &&
       !mongoc_write_concern_is_default (cursor->write_concern) &&
       server_stream->sd->max_wire_version >= WIRE_VERSION_CMD_WRITE_CONCERN) {
      mongoc_write_concern_append (cursor->write_concern, &parts.extra);
   }

   ret = mongoc_cluster_run_command_monitored (
      cluster, &parts, server_stream, reply, &cursor->error);

   /* Read and Write Concern Spec: "Drivers SHOULD parse server replies for a
    * "writeConcernError" field and report the error only in command-specific
    * helper methods that take a separate write concern parameter or an options
    * parameter that may contain a write concern option.
    *
    * Only command helpers with names like "_with_write_concern" can create
    * cursors with a non-NULL write_concern field.
    */
   if (ret && cursor->write_concern) {
      ret = !_mongoc_parse_wc_err (reply, &cursor->error);
   }

done:
   mongoc_server_stream_cleanup (server_stream);
   mongoc_cmd_parts_cleanup (&parts);

   return ret;
}


static bool
_translate_query_opt (const char *query_field, const char **cmd_field, int *len)
{
   if (query_field[0] != '$') {
      *cmd_field = query_field;
      *len = -1;
      return true;
   }

   /* strip the leading '$' */
   query_field++;

   if (!strcmp (MONGOC_CURSOR_ORDERBY, query_field)) {
      *cmd_field = MONGOC_CURSOR_SORT;
      *len = MONGOC_CURSOR_SORT_LEN;
   } else if (!strcmp (MONGOC_CURSOR_SHOW_DISK_LOC,
                       query_field)) { /* <= MongoDb 3.0 */
      *cmd_field = MONGOC_CURSOR_SHOW_RECORD_ID;
      *len = MONGOC_CURSOR_SHOW_RECORD_ID_LEN;
   } else if (!strcmp (MONGOC_CURSOR_HINT, query_field)) {
      *cmd_field = MONGOC_CURSOR_HINT;
      *len = MONGOC_CURSOR_HINT_LEN;
   } else if (!strcmp (MONGOC_CURSOR_COMMENT, query_field)) {
      *cmd_field = MONGOC_CURSOR_COMMENT;
      *len = MONGOC_CURSOR_COMMENT_LEN;
   } else if (!strcmp (MONGOC_CURSOR_MAX_SCAN, query_field)) {
      *cmd_field = MONGOC_CURSOR_MAX_SCAN;
      *len = MONGOC_CURSOR_MAX_SCAN_LEN;
   } else if (!strcmp (MONGOC_CURSOR_MAX_TIME_MS, query_field)) {
      *cmd_field = MONGOC_CURSOR_MAX_TIME_MS;
      *len = MONGOC_CURSOR_MAX_TIME_MS_LEN;
   } else if (!strcmp (MONGOC_CURSOR_MAX, query_field)) {
      *cmd_field = MONGOC_CURSOR_MAX;
      *len = MONGOC_CURSOR_MAX_LEN;
   } else if (!strcmp (MONGOC_CURSOR_MIN, query_field)) {
      *cmd_field = MONGOC_CURSOR_MIN;
      *len = MONGOC_CURSOR_MIN_LEN;
   } else if (!strcmp (MONGOC_CURSOR_RETURN_KEY, query_field)) {
      *cmd_field = MONGOC_CURSOR_RETURN_KEY;
      *len = MONGOC_CURSOR_RETURN_KEY_LEN;
   } else if (!strcmp (MONGOC_CURSOR_SNAPSHOT, query_field)) {
      *cmd_field = MONGOC_CURSOR_SNAPSHOT;
      *len = MONGOC_CURSOR_SNAPSHOT_LEN;
   } else {
      /* not a special command field, must be a query operator like $or */
      return false;
   }

   return true;
}


void
_mongoc_cursor_collection (const mongoc_cursor_t *cursor,
                           const char **collection,
                           int *collection_len)
{
   /* ns is like "db.collection". Collection name is located past the ".". */
   *collection = cursor->ns + (cursor->dblen + 1);
   /* Collection name's length is ns length, minus length of db name and ".". */
   *collection_len = cursor->nslen - cursor->dblen - 1;

   BSON_ASSERT (*collection_len > 0);
}


static bool
_mongoc_cursor_prepare_find_command (mongoc_cursor_t *cursor,
                                     bson_t *command,
                                     mongoc_server_stream_t *server_stream)
{
   const char *collection;
   int collection_len;
   bson_iter_t iter;

   _mongoc_cursor_collection (cursor, &collection, &collection_len);
   bson_append_utf8 (command,
                     MONGOC_CURSOR_FIND,
                     MONGOC_CURSOR_FIND_LEN,
                     collection,
                     collection_len);
   bson_append_document (
      command, MONGOC_CURSOR_FILTER, MONGOC_CURSOR_FILTER_LEN, &cursor->filter);
   bson_iter_init (&iter, &cursor->opts);

   while (bson_iter_next (&iter)) {
      /* don't append "maxAwaitTimeMS" */
      if (!strcmp (bson_iter_key (&iter), MONGOC_CURSOR_COLLATION) &&
          server_stream->sd->max_wire_version < WIRE_VERSION_COLLATION) {
         bson_set_error (&cursor->error,
                         MONGOC_ERROR_CURSOR,
                         MONGOC_ERROR_PROTOCOL_BAD_WIRE_VERSION,
                         "Collation is not supported by this server");
         MARK_FAILED (cursor);
         return false;
      } else if (strcmp (bson_iter_key (&iter),
                         MONGOC_CURSOR_MAX_AWAIT_TIME_MS)) {
         if (!bson_append_iter (command, bson_iter_key (&iter), -1, &iter)) {
            bson_set_error (&cursor->error,
                            MONGOC_ERROR_BSON,
                            MONGOC_ERROR_BSON_INVALID,
                            "Cursor opts too large");
            MARK_FAILED (cursor);
            return false;
         }
      }
   }

   if (cursor->read_concern->level != NULL) {
      const bson_t *read_concern_bson;

      read_concern_bson = _mongoc_read_concern_get_bson (cursor->read_concern);
      bson_append_document (command,
                            MONGOC_CURSOR_READ_CONCERN,
                            MONGOC_CURSOR_READ_CONCERN_LEN,
                            read_concern_bson);
   }

   return true;
}


static const bson_t *
_mongoc_cursor_find_command (mongoc_cursor_t *cursor,
                             mongoc_server_stream_t *server_stream)
{
   bson_t command = BSON_INITIALIZER;
   const bson_t *bson = NULL;

   ENTRY;

   if (!_mongoc_cursor_prepare_find_command (cursor, &command, server_stream)) {
      RETURN (NULL);
   }

   _mongoc_cursor_cursorid_init (cursor, &command);
   bson_destroy (&command);

   BSON_ASSERT (cursor->iface.next);
   _mongoc_cursor_cursorid_next (cursor, &bson);

   RETURN (bson);
}


static const bson_t *
_mongoc_cursor_get_more (mongoc_cursor_t *cursor)
{
   mongoc_server_stream_t *server_stream;
   const bson_t *b = NULL;

   ENTRY;

   BSON_ASSERT (cursor);

   server_stream = _mongoc_cursor_fetch_stream (cursor);
   if (!server_stream) {
      GOTO (failure);
   }

   if (!cursor->in_exhaust && !cursor->rpc.reply.cursor_id) {
      bson_set_error (&cursor->error,
                      MONGOC_ERROR_CURSOR,
                      MONGOC_ERROR_CURSOR_INVALID_CURSOR,
                      "No valid cursor was provided.");
      GOTO (failure);
   }

   if (!_mongoc_cursor_op_getmore (cursor, server_stream)) {
      GOTO (failure);
   }

   mongoc_server_stream_cleanup (server_stream);

   if (cursor->reader) {
      _mongoc_read_from_buffer (cursor, &b);
   }

   RETURN (b);

failure:
   cursor->done = true;

   mongoc_server_stream_cleanup (server_stream);

   RETURN (NULL);
}


static bool
_mongoc_cursor_monitor_legacy_get_more (mongoc_cursor_t *cursor,
                                        mongoc_server_stream_t *server_stream)
{
   bson_t doc;
   char db[MONGOC_NAMESPACE_MAX];
   mongoc_client_t *client;
   mongoc_apm_command_started_t event;

   ENTRY;

   client = cursor->client;
   if (!client->apm_callbacks.started) {
      /* successful */
      RETURN (true);
   }

   bson_init (&doc);
   if (!_mongoc_cursor_prepare_getmore_command (cursor, &doc)) {
      bson_destroy (&doc);
      RETURN (false);
   }

   bson_strncpy (db, cursor->ns, cursor->dblen + 1);
   mongoc_apm_command_started_init (&event,
                                    &doc,
                                    db,
                                    "getMore",
                                    client->cluster.request_id,
                                    cursor->operation_id,
                                    &server_stream->sd->host,
                                    server_stream->sd->id,
                                    client->apm_context);

   client->apm_callbacks.started (&event);
   mongoc_apm_command_started_cleanup (&event);
   bson_destroy (&doc);

   RETURN (true);
}


bool
_mongoc_cursor_op_getmore (mongoc_cursor_t *cursor,
                           mongoc_server_stream_t *server_stream)
{
   int64_t started;
   mongoc_rpc_t rpc;
   uint32_t request_id;
   mongoc_cluster_t *cluster;
   mongoc_query_flags_t flags;

   ENTRY;

   started = bson_get_monotonic_time ();
   cluster = &cursor->client->cluster;

   if (!_mongoc_cursor_flags (cursor, server_stream, &flags)) {
      GOTO (fail);
   }

   if (cursor->in_exhaust) {
      request_id = (uint32_t) cursor->rpc.header.request_id;
   } else {
      request_id = ++cluster->request_id;

      rpc.get_more.cursor_id = cursor->rpc.reply.cursor_id;
      rpc.header.msg_len = 0;
      rpc.header.request_id = request_id;
      rpc.header.response_to = 0;
      rpc.header.opcode = MONGOC_OPCODE_GET_MORE;
      rpc.get_more.zero = 0;
      rpc.get_more.collection = cursor->ns;

      if (flags & MONGOC_QUERY_TAILABLE_CURSOR) {
         rpc.get_more.n_return = 0;
      } else {
         rpc.get_more.n_return = _mongoc_n_return (cursor);
      }

      if (!_mongoc_cursor_monitor_legacy_get_more (cursor, server_stream)) {
         GOTO (fail);
      }

      if (!mongoc_cluster_sendv_to_server (
             cluster, &rpc, server_stream, NULL, &cursor->error)) {
         GOTO (fail);
      }
   }

   _mongoc_buffer_clear (&cursor->buffer, false);

   if (!_mongoc_client_recv (cursor->client,
                             &cursor->rpc,
                             &cursor->buffer,
                             server_stream,
                             &cursor->error)) {
      GOTO (fail);
   }

   if (cursor->rpc.header.opcode != MONGOC_OPCODE_REPLY) {
      bson_set_error (&cursor->error,
                      MONGOC_ERROR_PROTOCOL,
                      MONGOC_ERROR_PROTOCOL_INVALID_REPLY,
                      "Invalid opcode. Expected %d, got %d.",
                      MONGOC_OPCODE_REPLY,
                      cursor->rpc.header.opcode);
      GOTO (fail);
   }

   if (cursor->rpc.header.response_to != request_id) {
      bson_set_error (&cursor->error,
                      MONGOC_ERROR_PROTOCOL,
                      MONGOC_ERROR_PROTOCOL_INVALID_REPLY,
                      "Invalid response_to for getmore. Expected %d, got %d.",
                      request_id,
                      cursor->rpc.header.response_to);
      GOTO (fail);
   }

   if (!_mongoc_rpc_check_ok (&cursor->rpc,
                              false /* is_command */,
                              cursor->client->error_api_version,
                              &cursor->error,
                              &cursor->error_doc)) {
      GOTO (fail);
   }

   if (cursor->reader) {
      bson_reader_destroy (cursor->reader);
   }

   cursor->reader = bson_reader_new_from_data (
      cursor->rpc.reply.documents, (size_t) cursor->rpc.reply.documents_len);

   _mongoc_cursor_monitor_succeeded (cursor,
                                     bson_get_monotonic_time () - started,
                                     false, /* not first batch */
                                     server_stream,
                                     "getMore");

   RETURN (true);

fail:
   _mongoc_cursor_monitor_failed (
      cursor, bson_get_monotonic_time () - started, server_stream, "getMore");
   RETURN (false);
}


bool
mongoc_cursor_error (mongoc_cursor_t *cursor, bson_error_t *error)
{
   ENTRY;

   RETURN (mongoc_cursor_error_document (cursor, error, NULL));
}


bool
mongoc_cursor_error_document (mongoc_cursor_t *cursor,
                              bson_error_t *error,
                              const bson_t **doc)
{
   bool ret;

   ENTRY;

   BSON_ASSERT (cursor);

   if (cursor->iface.error_document) {
      ret = cursor->iface.error_document (cursor, error, doc);
   } else {
      ret = _mongoc_cursor_error_document (cursor, error, doc);
   }

   RETURN (ret);
}


bool
_mongoc_cursor_error_document (mongoc_cursor_t *cursor,
                               bson_error_t *error,
                               const bson_t **doc)
{
   ENTRY;

   BSON_ASSERT (cursor);

   if (BSON_UNLIKELY (CURSOR_FAILED (cursor))) {
      bson_set_error (error,
                      cursor->error.domain,
                      cursor->error.code,
                      "%s",
                      cursor->error.message);

      if (doc) {
         *doc = &cursor->error_doc;
      }

      RETURN (true);
   }

   if (doc) {
      *doc = NULL;
   }

   RETURN (false);
}


bool
mongoc_cursor_next (mongoc_cursor_t *cursor, const bson_t **bson)
{
   bool ret;

   ENTRY;

   BSON_ASSERT (cursor);
   BSON_ASSERT (bson);

   TRACE ("cursor_id(%" PRId64 ")", cursor->rpc.reply.cursor_id);

   if (bson) {
      *bson = NULL;
   }

   if (CURSOR_FAILED (cursor)) {
      return false;
   }

   if (cursor->done) {
      bson_set_error (&cursor->error,
                      MONGOC_ERROR_CURSOR,
                      MONGOC_ERROR_CURSOR_INVALID_CURSOR,
                      "Cannot advance a completed or failed cursor.");
      return false;
   }

   /*
    * We cannot proceed if another cursor is receiving results in exhaust mode.
    */
   if (cursor->client->in_exhaust && !cursor->in_exhaust) {
      bson_set_error (&cursor->error,
                      MONGOC_ERROR_CLIENT,
                      MONGOC_ERROR_CLIENT_IN_EXHAUST,
                      "Another cursor derived from this client is in exhaust.");
      RETURN (false);
   }

   if (cursor->iface.next) {
      ret = cursor->iface.next (cursor, bson);
   } else {
      ret = _mongoc_cursor_next (cursor, bson);
   }

   cursor->current = *bson;

   cursor->count++;

   RETURN (ret);
}


bool
_mongoc_read_from_buffer (mongoc_cursor_t *cursor, const bson_t **bson)
{
   bool eof = false;

   BSON_ASSERT (cursor->reader);

   *bson = bson_reader_read (cursor->reader, &eof);
   cursor->end_of_event = eof ? 1 : 0;

   return *bson ? true : false;
}


bool
_mongoc_cursor_next (mongoc_cursor_t *cursor, const bson_t **bson)
{
   int64_t limit;
   const bson_t *b = NULL;
   bool tailable;

   ENTRY;

   BSON_ASSERT (cursor);

   if (bson) {
      *bson = NULL;
   }

   /*
    * If we reached our limit, make sure we mark this as done and do not try to
    * make further progress.  We also set end_of_event so that
    * mongoc_cursor_more will be false.
    */
   limit = mongoc_cursor_get_limit (cursor);

   if (limit && cursor->count >= llabs (limit)) {
      cursor->done = true;
      cursor->end_of_event = true;
      RETURN (false);
   }

   /*
    * Try to read the next document from the reader if it exists, we might
    * get NULL back and EOF, in which case we need to submit a getmore.
    */
   if (cursor->reader) {
      _mongoc_read_from_buffer (cursor, &b);
      if (b) {
         GOTO (complete);
      }
   }

   /*
    * Check to see if we need to send a GET_MORE for more results.
    */
   if (!cursor->sent) {
      b = _mongoc_cursor_initial_query (cursor);
   } else if (BSON_UNLIKELY (cursor->end_of_event) &&
              cursor->rpc.reply.cursor_id) {
      b = _mongoc_cursor_get_more (cursor);
   }

complete:
   tailable = _mongoc_cursor_get_opt_bool (cursor, "tailable");
   cursor->done = (cursor->end_of_event &&
                   ((cursor->in_exhaust && !cursor->rpc.reply.cursor_id) ||
                    (!b && !tailable)));

   if (bson) {
      *bson = b;
   }

   RETURN (!!b);
}


bool
mongoc_cursor_more (mongoc_cursor_t *cursor)
{
   bool ret;

   ENTRY;

   BSON_ASSERT (cursor);

   if (cursor->iface.more) {
      ret = cursor->iface.more (cursor);
   } else {
      ret = _mongoc_cursor_more (cursor);
   }

   RETURN (ret);
}


bool
_mongoc_cursor_more (mongoc_cursor_t *cursor)
{
   BSON_ASSERT (cursor);

   if (CURSOR_FAILED (cursor)) {
      return false;
   }

   return !(cursor->sent && cursor->done && cursor->end_of_event);
}


void
mongoc_cursor_get_host (mongoc_cursor_t *cursor, mongoc_host_list_t *host)
{
   BSON_ASSERT (cursor);
   BSON_ASSERT (host);

   if (cursor->iface.get_host) {
      cursor->iface.get_host (cursor, host);
   } else {
      _mongoc_cursor_get_host (cursor, host);
   }

   EXIT;
}

void
_mongoc_cursor_get_host (mongoc_cursor_t *cursor, mongoc_host_list_t *host)
{
   mongoc_server_description_t *description;

   BSON_ASSERT (cursor);
   BSON_ASSERT (host);

   memset (host, 0, sizeof *host);

   if (!cursor->server_id) {
      MONGOC_WARNING ("%s(): Must send query before fetching peer.", BSON_FUNC);
      return;
   }

   description = mongoc_topology_server_by_id (
      cursor->client->topology, cursor->server_id, &cursor->error);
   if (!description) {
      return;
   }

   *host = description->host;

   mongoc_server_description_destroy (description);

   return;
}

mongoc_cursor_t *
mongoc_cursor_clone (const mongoc_cursor_t *cursor)
{
   mongoc_cursor_t *ret;

   BSON_ASSERT (cursor);

   if (cursor->iface.clone) {
      ret = cursor->iface.clone (cursor);
   } else {
      ret = _mongoc_cursor_clone (cursor);
   }

   RETURN (ret);
}


mongoc_cursor_t *
_mongoc_cursor_clone (const mongoc_cursor_t *cursor)
{
   mongoc_cursor_t *_clone;

   ENTRY;

   BSON_ASSERT (cursor);

   _clone = (mongoc_cursor_t *) bson_malloc0 (sizeof *_clone);

   _clone->client = cursor->client;
   _clone->is_command = cursor->is_command;
   _clone->nslen = cursor->nslen;
   _clone->dblen = cursor->dblen;
   _clone->has_fields = cursor->has_fields;

   if (cursor->read_prefs) {
      _clone->read_prefs = mongoc_read_prefs_copy (cursor->read_prefs);
   }

   if (cursor->read_concern) {
      _clone->read_concern = mongoc_read_concern_copy (cursor->read_concern);
   }


   bson_copy_to (&cursor->filter, &_clone->filter);
   bson_copy_to (&cursor->opts, &_clone->opts);
   bson_copy_to (&cursor->error_doc, &_clone->error_doc);

   bson_strncpy (_clone->ns, cursor->ns, sizeof _clone->ns);

   _mongoc_buffer_init (&_clone->buffer, NULL, 0, NULL, NULL);

   mongoc_counter_cursors_active_inc ();

   RETURN (_clone);
}


/*
 *--------------------------------------------------------------------------
 *
 * mongoc_cursor_is_alive --
 *
 *       Checks to see if a cursor is alive.
 *
 *       This is primarily useful with tailable cursors.
 *
 * Returns:
 *       true if the cursor is alive.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

bool
mongoc_cursor_is_alive (const mongoc_cursor_t *cursor) /* IN */
{
   BSON_ASSERT (cursor);

   return !cursor->done;
}


const bson_t *
mongoc_cursor_current (const mongoc_cursor_t *cursor) /* IN */
{
   BSON_ASSERT (cursor);

   return cursor->current;
}


void
mongoc_cursor_set_batch_size (mongoc_cursor_t *cursor, uint32_t batch_size)
{
   BSON_ASSERT (cursor);

   _mongoc_cursor_set_opt_int64 (
      cursor, MONGOC_CURSOR_BATCH_SIZE, (int64_t) batch_size);
}

uint32_t
mongoc_cursor_get_batch_size (const mongoc_cursor_t *cursor)
{
   BSON_ASSERT (cursor);

   return (uint32_t) _mongoc_cursor_get_opt_int64 (
      cursor, MONGOC_CURSOR_BATCH_SIZE, 0);
}

bool
mongoc_cursor_set_limit (mongoc_cursor_t *cursor, int64_t limit)
{
   BSON_ASSERT (cursor);

   if (!cursor->sent) {
      if (limit < 0) {
         return _mongoc_cursor_set_opt_int64 (
                   cursor, MONGOC_CURSOR_LIMIT, -limit) &&
                _mongoc_cursor_set_opt_bool (
                   cursor, MONGOC_CURSOR_SINGLE_BATCH, true);
      } else {
         return _mongoc_cursor_set_opt_int64 (
            cursor, MONGOC_CURSOR_LIMIT, limit);
      }
   } else {
      return false;
   }
}

int64_t
mongoc_cursor_get_limit (const mongoc_cursor_t *cursor)
{
   int64_t limit;
   bool single_batch;

   BSON_ASSERT (cursor);

   limit = _mongoc_cursor_get_opt_int64 (cursor, MONGOC_CURSOR_LIMIT, 0);
   single_batch =
      _mongoc_cursor_get_opt_bool (cursor, MONGOC_CURSOR_SINGLE_BATCH);

   if (limit > 0 && single_batch) {
      limit = -limit;
   }

   return limit;
}

bool
mongoc_cursor_set_hint (mongoc_cursor_t *cursor, uint32_t server_id)
{
   BSON_ASSERT (cursor);

   if (cursor->server_id) {
      MONGOC_ERROR ("mongoc_cursor_set_hint: server_id already set");
      return false;
   }

   if (!server_id) {
      MONGOC_ERROR ("mongoc_cursor_set_hint: cannot set server_id to 0");
      return false;
   }

   cursor->server_id = server_id;
   cursor->server_id_set = true;

   return true;
}

uint32_t
mongoc_cursor_get_hint (const mongoc_cursor_t *cursor)
{
   BSON_ASSERT (cursor);

   return cursor->server_id;
}

int64_t
mongoc_cursor_get_id (const mongoc_cursor_t *cursor)
{
   BSON_ASSERT (cursor);

   return cursor->rpc.reply.cursor_id;
}

void
mongoc_cursor_set_max_await_time_ms (mongoc_cursor_t *cursor,
                                     uint32_t max_await_time_ms)
{
   BSON_ASSERT (cursor);

   if (!cursor->sent) {
      _mongoc_cursor_set_opt_int64 (
         cursor, MONGOC_CURSOR_MAX_AWAIT_TIME_MS, (int64_t) max_await_time_ms);
   }
}

uint32_t
mongoc_cursor_get_max_await_time_ms (const mongoc_cursor_t *cursor)
{
   bson_iter_t iter;

   BSON_ASSERT (cursor);

   if (bson_iter_init_find (
          &iter, &cursor->opts, MONGOC_CURSOR_MAX_AWAIT_TIME_MS)) {
      return (uint32_t) bson_iter_as_int64 (&iter);
   }

   return 0;
}


/*
 *--------------------------------------------------------------------------
 *
 * mongoc_cursor_new_from_command_reply --
 *
 *       Low-level function to initialize a mongoc_cursor_t from the
 *       reply to a command like "aggregate", "find", or "listCollections".
 *
 *       Useful in drivers that wrap the C driver; in applications, use
 *       high-level functions like mongoc_collection_aggregate instead.
 *
 * Returns:
 *       A cursor.
 *
 * Side effects:
 *       On failure, the cursor's error is set: retrieve it with
 *       mongoc_cursor_error. On success or failure, "reply" is
 *       destroyed.
 *
 *--------------------------------------------------------------------------
 */

mongoc_cursor_t *
mongoc_cursor_new_from_command_reply (mongoc_client_t *client,
                                      bson_t *reply,
                                      uint32_t server_id)
{
   mongoc_cursor_t *cursor;
   bson_t cmd = BSON_INITIALIZER;

   BSON_ASSERT (client);
   BSON_ASSERT (reply);

   cursor = _mongoc_cursor_new_with_opts (
      client, NULL, false /* is_command */, NULL, NULL, NULL, NULL);

   _mongoc_cursor_cursorid_init (cursor, &cmd);
   _mongoc_cursor_cursorid_init_with_reply (cursor, reply, server_id);
   bson_destroy (&cmd);

   return cursor;
}
