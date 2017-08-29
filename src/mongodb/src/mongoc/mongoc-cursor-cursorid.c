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
#include "mongoc-cursor-cursorid-private.h"
#include "mongoc-log.h"
#include "mongoc-trace-private.h"
#include "mongoc-error.h"
#include "mongoc-util-private.h"
#include "mongoc-client-private.h"


#undef MONGOC_LOG_DOMAIN
#define MONGOC_LOG_DOMAIN "cursor-cursorid"


static void *
_mongoc_cursor_cursorid_new (void)
{
   mongoc_cursor_cursorid_t *cid;

   ENTRY;

   cid = (mongoc_cursor_cursorid_t *) bson_malloc0 (sizeof *cid);
   bson_init (&cid->array);
   cid->in_batch = false;
   cid->in_reader = false;

   RETURN (cid);
}


static void
_mongoc_cursor_cursorid_destroy (mongoc_cursor_t *cursor)
{
   mongoc_cursor_cursorid_t *cid;

   ENTRY;

   cid = (mongoc_cursor_cursorid_t *) cursor->iface_data;
   BSON_ASSERT (cid);

   bson_destroy (&cid->array);
   bson_free (cid);
   _mongoc_cursor_destroy (cursor);

   EXIT;
}


/*
 * Start iterating the reply to an "aggregate", "find", "getMore" etc. command:
 *
 *    {cursor: {id: 1234, ns: "db.collection", firstBatch: [...]}}
 */
bool
_mongoc_cursor_cursorid_start_batch (mongoc_cursor_t *cursor)
{
   mongoc_cursor_cursorid_t *cid;
   bson_iter_t iter;
   bson_iter_t child;
   const char *ns;
   uint32_t nslen;

   cid = (mongoc_cursor_cursorid_t *) cursor->iface_data;

   BSON_ASSERT (cid);

   if (bson_iter_init_find (&iter, &cid->array, "cursor") &&
       BSON_ITER_HOLDS_DOCUMENT (&iter) && bson_iter_recurse (&iter, &child)) {
      while (bson_iter_next (&child)) {
         if (BSON_ITER_IS_KEY (&child, "id")) {
            cursor->rpc.reply.cursor_id = bson_iter_as_int64 (&child);
         } else if (BSON_ITER_IS_KEY (&child, "ns")) {
            ns = bson_iter_utf8 (&child, &nslen);
            _mongoc_set_cursor_ns (cursor, ns, nslen);
         } else if (BSON_ITER_IS_KEY (&child, "firstBatch") ||
                    BSON_ITER_IS_KEY (&child, "nextBatch")) {
            if (BSON_ITER_HOLDS_ARRAY (&child) &&
                bson_iter_recurse (&child, &cid->batch_iter)) {
               cid->in_batch = true;
            }
         }
      }
   }

   return cid->in_batch;
}


static bool
_mongoc_cursor_cursorid_refresh_from_command (mongoc_cursor_t *cursor,
                                              const bson_t *command)
{
   mongoc_cursor_cursorid_t *cid;

   ENTRY;

   cid = (mongoc_cursor_cursorid_t *) cursor->iface_data;
   BSON_ASSERT (cid);

   bson_destroy (&cid->array);

   /* server replies to find / aggregate with {cursor: {id: N, firstBatch: []}},
    * to getMore command with {cursor: {id: N, nextBatch: []}}. */
   if (_mongoc_cursor_run_command (cursor, command, &cid->array) &&
       _mongoc_cursor_cursorid_start_batch (cursor)) {
      RETURN (true);
   }

   bson_destroy (&cursor->error_doc);
   bson_copy_to (&cid->array, &cursor->error_doc);

   if (!cursor->error.domain) {
      bson_set_error (&cursor->error,
                      MONGOC_ERROR_PROTOCOL,
                      MONGOC_ERROR_PROTOCOL_INVALID_REPLY,
                      "Invalid reply to %s command.",
                      _mongoc_get_command_name (command));
   }

   RETURN (false);
}


static void
_mongoc_cursor_cursorid_read_from_batch (mongoc_cursor_t *cursor,
                                         const bson_t **bson)
{
   mongoc_cursor_cursorid_t *cid;
   const uint8_t *data = NULL;
   uint32_t data_len = 0;

   ENTRY;

   cid = (mongoc_cursor_cursorid_t *) cursor->iface_data;
   BSON_ASSERT (cid);

   if (bson_iter_next (&cid->batch_iter) &&
       BSON_ITER_HOLDS_DOCUMENT (&cid->batch_iter)) {
      bson_iter_document (&cid->batch_iter, &data_len, &data);

      /* bson_iter_next guarantees valid BSON, so this must succeed */
      bson_init_static (&cid->current_doc, data, data_len);
      *bson = &cid->current_doc;

      cursor->end_of_event = false;
   } else {
      cursor->end_of_event = true;
   }
}


bool
_mongoc_cursor_cursorid_prime (mongoc_cursor_t *cursor)
{
   cursor->sent = true;
   cursor->operation_id = ++cursor->client->cluster.operation_id;
   return _mongoc_cursor_cursorid_refresh_from_command (cursor,
                                                        &cursor->filter);
}


bool
_mongoc_cursor_prepare_getmore_command (mongoc_cursor_t *cursor,
                                        bson_t *command)
{
   const char *collection;
   int collection_len;
   int64_t batch_size;
   bool await_data;
   int32_t max_await_time_ms;

   ENTRY;

   _mongoc_cursor_collection (cursor, &collection, &collection_len);

   bson_init (command);
   bson_append_int64 (command, "getMore", 7, mongoc_cursor_get_id (cursor));
   bson_append_utf8 (command, "collection", 10, collection, collection_len);

   batch_size = mongoc_cursor_get_batch_size (cursor);

   /* See find, getMore, and killCursors Spec for batchSize rules */
   if (batch_size) {
      bson_append_int64 (command,
                         MONGOC_CURSOR_BATCH_SIZE,
                         MONGOC_CURSOR_BATCH_SIZE_LEN,
                         abs (_mongoc_n_return (cursor)));
   }

   /* Find, getMore And killCursors Commands Spec: "In the case of a tailable
      cursor with awaitData == true the driver MUST provide a Cursor level
      option named maxAwaitTimeMS (See CRUD specification for details). The
      maxTimeMS option on the getMore command MUST be set to the value of the
      option maxAwaitTimeMS. If no maxAwaitTimeMS is specified, the driver
      SHOULD not set maxTimeMS on the getMore command."
    */
   await_data = _mongoc_cursor_get_opt_bool (cursor, MONGOC_CURSOR_TAILABLE) &&
                _mongoc_cursor_get_opt_bool (cursor, MONGOC_CURSOR_AWAIT_DATA);


   if (await_data) {
      max_await_time_ms =
         (int32_t) mongoc_cursor_get_max_await_time_ms (cursor);
      if (max_await_time_ms) {
         bson_append_int32 (command,
                            MONGOC_CURSOR_MAX_TIME_MS,
                            MONGOC_CURSOR_MAX_TIME_MS_LEN,
                            max_await_time_ms);
      }
   }

   RETURN (true);
}


static bool
_mongoc_cursor_cursorid_get_more (mongoc_cursor_t *cursor)
{
   mongoc_cursor_cursorid_t *cid;
   mongoc_server_stream_t *server_stream;
   bson_t command;
   bool ret;

   ENTRY;

   cid = (mongoc_cursor_cursorid_t *) cursor->iface_data;
   BSON_ASSERT (cid);

   server_stream = _mongoc_cursor_fetch_stream (cursor);

   if (!server_stream) {
      RETURN (false);
   }

   if (_use_getmore_command (cursor, server_stream)) {
      if (!_mongoc_cursor_prepare_getmore_command (cursor, &command)) {
         mongoc_server_stream_cleanup (server_stream);
         RETURN (false);
      }

      ret = _mongoc_cursor_cursorid_refresh_from_command (cursor, &command);
      bson_destroy (&command);
   } else {
      ret = _mongoc_cursor_op_getmore (cursor, server_stream);
      cid->in_reader = ret;
   }

   mongoc_server_stream_cleanup (server_stream);
   RETURN (ret);
}


bool
_mongoc_cursor_cursorid_next (mongoc_cursor_t *cursor, const bson_t **bson)
{
   mongoc_cursor_cursorid_t *cid;
   bool refreshed = false;

   ENTRY;

   *bson = NULL;

   cid = (mongoc_cursor_cursorid_t *) cursor->iface_data;
   BSON_ASSERT (cid);

   if (!cursor->sent) {
      if (!_mongoc_cursor_cursorid_prime (cursor)) {
         GOTO (done);
      }
   }

again:

   /* Two paths:
    * - Mongo 3.2+, sent "getMore" cmd, we're reading reply's "nextBatch" array
    * - Mongo 2.6 to 3, after "aggregate" or similar command we sent OP_GETMORE,
    *   we're reading the raw reply
    */
   if (cid->in_batch) {
      _mongoc_cursor_cursorid_read_from_batch (cursor, bson);

      if (*bson) {
         GOTO (done);
      }

      cid->in_batch = false;
   } else if (cid->in_reader) {
      _mongoc_read_from_buffer (cursor, bson);

      if (*bson) {
         GOTO (done);
      }

      cid->in_reader = false;
   }

   if (!refreshed && mongoc_cursor_get_id (cursor)) {
      if (!_mongoc_cursor_cursorid_get_more (cursor)) {
         GOTO (done);
      }

      refreshed = true;
      GOTO (again);
   }

done:
   if (!*bson && mongoc_cursor_get_id (cursor) == 0) {
      cursor->done = 1;
   }

   RETURN (*bson != NULL);
}


static mongoc_cursor_t *
_mongoc_cursor_cursorid_clone (const mongoc_cursor_t *cursor)
{
   mongoc_cursor_t *clone_;

   ENTRY;

   clone_ = _mongoc_cursor_clone (cursor);
   _mongoc_cursor_cursorid_init (clone_, &cursor->filter);

   RETURN (clone_);
}


static mongoc_cursor_interface_t gMongocCursorCursorid = {
   _mongoc_cursor_cursorid_clone,
   _mongoc_cursor_cursorid_destroy,
   NULL,
   _mongoc_cursor_cursorid_next,
};


void
_mongoc_cursor_cursorid_init (mongoc_cursor_t *cursor, const bson_t *command)
{
   ENTRY;

   bson_destroy (&cursor->filter);
   bson_copy_to (command, &cursor->filter);

   cursor->iface_data = _mongoc_cursor_cursorid_new ();

   memcpy (&cursor->iface,
           &gMongocCursorCursorid,
           sizeof (mongoc_cursor_interface_t));

   EXIT;
}

void
_mongoc_cursor_cursorid_init_with_reply (mongoc_cursor_t *cursor,
                                         bson_t *reply,
                                         uint32_t server_id)
{
   mongoc_cursor_cursorid_t *cid;

   cursor->sent = true;
   cursor->server_id = server_id;

   cid = (mongoc_cursor_cursorid_t *) cursor->iface_data;
   BSON_ASSERT (cid);

   bson_destroy (&cid->array);
   if (!bson_steal (&cid->array, reply)) {
      bson_steal (&cid->array, bson_copy (reply));
   }

   if (!_mongoc_cursor_cursorid_start_batch (cursor)) {
      bson_set_error (&cursor->error,
                      MONGOC_ERROR_CURSOR,
                      MONGOC_ERROR_CURSOR_INVALID_CURSOR,
                      "Couldn't parse cursor document");
   }
}
