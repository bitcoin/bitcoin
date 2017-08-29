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


#include <mongoc-rpc-private.h>
#include "mongoc.h"

#include "mock-server.h"
#include "../test-conveniences.h"
#include "../TestSuite.h"


static bool
is_command_ns (const char *ns);

static void
request_from_query (request_t *request, const mongoc_rpc_t *rpc);

static void
request_from_insert (request_t *request, const mongoc_rpc_t *rpc);

static void
request_from_update (request_t *request, const mongoc_rpc_t *rpc);

static void
request_from_delete (request_t *request, const mongoc_rpc_t *rpc);

static void
request_from_killcursors (request_t *request, const mongoc_rpc_t *rpc);

static void
request_from_getmore (request_t *request, const mongoc_rpc_t *rpc);

static char *
query_flags_str (uint32_t flags);
static char *
insert_flags_str (uint32_t flags);
static char *
update_flags_str (uint32_t flags);
static char *
delete_flags_str (uint32_t flags);

request_t *
request_new (const mongoc_buffer_t *buffer,
             int32_t msg_len,
             mock_server_t *server,
             mongoc_stream_t *client,
             uint16_t client_port,
             sync_queue_t *replies)
{
   request_t *request = (request_t *) bson_malloc0 (sizeof *request);
   uint8_t *data;

   data = (uint8_t *) bson_malloc ((size_t) msg_len);
   memcpy (data, buffer->data + buffer->off, (size_t) msg_len);
   request->data = data;
   request->data_len = (size_t) msg_len;
   request->replies = replies;

   if (!_mongoc_rpc_scatter (&request->request_rpc, data, (size_t) msg_len)) {
      MONGOC_WARNING ("%s():%d: %s", BSON_FUNC, __LINE__, "Failed to scatter");
      bson_free (data);
      bson_free (request);
      return NULL;
   }

   _mongoc_rpc_swab_from_le (&request->request_rpc);

   request->opcode = (mongoc_opcode_t) request->request_rpc.header.opcode;
   request->server = server;
   request->client = client;
   request->client_port = client_port;
   _mongoc_array_init (&request->docs, sizeof (bson_t *));

   switch (request->opcode) {
   case MONGOC_OPCODE_COMPRESSED:
      break;
   case MONGOC_OPCODE_QUERY:
      request_from_query (request, &request->request_rpc);
      break;

   case MONGOC_OPCODE_INSERT:
      request_from_insert (request, &request->request_rpc);
      break;

   case MONGOC_OPCODE_UPDATE:
      request_from_update (request, &request->request_rpc);
      break;

   case MONGOC_OPCODE_KILL_CURSORS:
      request_from_killcursors (request, &request->request_rpc);
      break;

   case MONGOC_OPCODE_GET_MORE:
      request_from_getmore (request, &request->request_rpc);
      break;

   case MONGOC_OPCODE_DELETE:
      request_from_delete (request, &request->request_rpc);
      break;

   case MONGOC_OPCODE_REPLY:
   case MONGOC_OPCODE_MSG:
   default:
      fprintf (stderr, "Unimplemented opcode %d\n", request->opcode);
      abort ();
   }

   return request;
}

const bson_t *
request_get_doc (const request_t *request, int n)
{
   return _mongoc_array_index (&request->docs, const bson_t *, n);
}

bool
request_matches_flags (const request_t *request, mongoc_query_flags_t flags)
{
   const mongoc_rpc_t *rpc;

   BSON_ASSERT (request);
   rpc = &request->request_rpc;

   if (rpc->query.flags != flags) {
      test_error ("request's query flags are %s, expected %s",
                  query_flags_str (rpc->query.flags),
                  query_flags_str (flags));
      return false;
   }

   return true;
}

/* TODO: take file, line, function params from caller, wrap in macro */
bool
request_matches_query (const request_t *request,
                       const char *ns,
                       mongoc_query_flags_t flags,
                       uint32_t skip,
                       int32_t n_return,
                       const char *query_json,
                       const char *fields_json,
                       bool is_command)
{
   const mongoc_rpc_t *rpc;
   const bson_t *doc;
   bool n_return_equal;

   BSON_ASSERT (request);
   rpc = &request->request_rpc;

   BSON_ASSERT (request->docs.len <= 2);

   if (request->is_command && !is_command) {
      test_error ("expected query, got command");
      return false;
   }

   if (!request->is_command && is_command) {
      test_error ("expected command, got query");
      return false;
   }

   if (request->opcode != MONGOC_OPCODE_QUERY) {
      test_error ("request's opcode does not match QUERY");
      return false;
   }

   if (strcmp (rpc->query.collection, ns)) {
      test_error ("request's namespace is '%s', expected '%s'",
                  request->request_rpc.query.collection,
                  ns);
      return false;
   }

   if (!request_matches_flags (request, flags)) {
      return false;
   }

   if (rpc->query.skip != skip) {
      test_error ("requests's skip = %d, expected %d", rpc->query.skip, skip);
      return false;
   }

   n_return_equal = (rpc->query.n_return == n_return);

   if (!n_return_equal && abs (rpc->query.n_return) == 1) {
      /* quirk: commands from mongoc_client_command_simple have n_return 1,
       * from mongoc_topology_scanner_t have n_return -1
       */
      n_return_equal = abs (rpc->query.n_return) == n_return;
   }

   if (!n_return_equal) {
      test_error ("requests's n_return = %d, expected %d",
                  rpc->query.n_return,
                  n_return);
      return false;
   }

   if (request->docs.len) {
      doc = request_get_doc (request, 0);
   } else {
      doc = NULL;
   }

   if (!match_json (
          doc, is_command, __FILE__, __LINE__, BSON_FUNC, query_json)) {
      /* match_json has logged the err */
      return false;
   }

   if (request->docs.len > 1) {
      doc = request_get_doc (request, 1);
   } else {
      doc = NULL;
   }

   if (!match_json (doc, false, __FILE__, __LINE__, BSON_FUNC, fields_json)) {
      /* match_json has logged the err */
      return false;
   }

   return true;
}


/* TODO: take file, line, function params from caller, wrap in macro */
bool
request_matches_insert (const request_t *request,
                        const char *ns,
                        mongoc_insert_flags_t flags,
                        const char *doc_json)
{
   const mongoc_rpc_t *rpc;
   const bson_t *doc;

   BSON_ASSERT (request);
   rpc = &request->request_rpc;

   if (request->opcode != MONGOC_OPCODE_INSERT) {
      test_error ("request's opcode does not match INSERT");
      return false;
   }

   if (strcmp (rpc->insert.collection, ns)) {
      test_error ("insert's namespace is '%s', expected '%s'",
                  request->request_rpc.get_more.collection,
                  ns);
      return false;
   }

   if (rpc->insert.flags != flags) {
      test_error ("request's insert flags are %s, expected %s",
                  insert_flags_str (rpc->insert.flags),
                  insert_flags_str (flags));
      return false;
   }

   ASSERT_CMPINT ((int) request->docs.len, ==, 1);
   doc = request_get_doc (request, 0);
   if (!match_json (doc, false, __FILE__, __LINE__, BSON_FUNC, doc_json)) {
      return false;
   }

   return true;
}


/* TODO: take file, line, function params from caller, wrap in macro */
bool
request_matches_bulk_insert (const request_t *request,
                             const char *ns,
                             mongoc_insert_flags_t flags,
                             int n)
{
   const mongoc_rpc_t *rpc;

   BSON_ASSERT (request);
   rpc = &request->request_rpc;

   if (request->opcode != MONGOC_OPCODE_INSERT) {
      test_error ("request's opcode does not match INSERT");
      return false;
   }

   if (strcmp (rpc->insert.collection, ns)) {
      test_error ("insert's namespace is '%s', expected '%s'",
                  request->request_rpc.get_more.collection,
                  ns);
      return false;
   }

   if (rpc->insert.flags != flags) {
      test_error ("request's insert flags are %s, expected %s",
                  insert_flags_str (rpc->insert.flags),
                  insert_flags_str (flags));
      return false;
   }

   if ((int) request->docs.len != n) {
      test_error (
         "expected %d docs inserted, got %d", n, (int) request->docs.len);
      return false;
   }

   return true;
}


/* TODO: take file, line, function params from caller, wrap in macro */
bool
request_matches_update (const request_t *request,
                        const char *ns,
                        mongoc_update_flags_t flags,
                        const char *selector_json,
                        const char *update_json)
{
   const mongoc_rpc_t *rpc;
   const bson_t *doc;

   BSON_ASSERT (request);
   rpc = &request->request_rpc;

   if (request->opcode != MONGOC_OPCODE_UPDATE) {
      test_error ("request's opcode does not match UPDATE");
      return false;
   }

   if (strcmp (rpc->update.collection, ns)) {
      test_error ("update's namespace is '%s', expected '%s'",
                  request->request_rpc.update.collection,
                  ns);
      return false;
   }

   if (rpc->update.flags != flags) {
      test_error ("request's update flags are %s, expected %s",
                  update_flags_str (rpc->update.flags),
                  update_flags_str (flags));
      return false;
   }

   ASSERT_CMPINT ((int) request->docs.len, ==, 2);
   doc = request_get_doc (request, 0);
   if (!match_json (doc, false, __FILE__, __LINE__, BSON_FUNC, selector_json)) {
      return false;
   }

   doc = request_get_doc (request, 1);
   if (!match_json (doc, false, __FILE__, __LINE__, BSON_FUNC, update_json)) {
      return false;
   }

   return true;
}


/* TODO: take file, line, function params from caller, wrap in macro */
bool
request_matches_delete (const request_t *request,
                        const char *ns,
                        mongoc_remove_flags_t flags,
                        const char *selector_json)
{
   const mongoc_rpc_t *rpc;
   const bson_t *doc;

   BSON_ASSERT (request);
   rpc = &request->request_rpc;

   if (request->opcode != MONGOC_OPCODE_DELETE) {
      test_error ("request's opcode does not match DELETE");
      return false;
   }

   if (strcmp (rpc->delete_.collection, ns)) {
      test_error ("delete's namespace is '%s', expected '%s'",
                  request->request_rpc.delete_.collection,
                  ns);
      return false;
   }

   if (rpc->delete_.flags != flags) {
      test_error ("request's delete flags are %s, expected %s",
                  delete_flags_str (rpc->delete_.flags),
                  delete_flags_str (flags));
      return false;
   }

   ASSERT_CMPINT ((int) request->docs.len, ==, 1);
   doc = request_get_doc (request, 0);
   if (!match_json (doc, false, __FILE__, __LINE__, BSON_FUNC, selector_json)) {
      return false;
   }

   return true;
}


/* TODO: take file, line, function params from caller, wrap in macro */
bool
request_matches_getmore (const request_t *request,
                         const char *ns,
                         int32_t n_return,
                         int64_t cursor_id)
{
   const mongoc_rpc_t *rpc;

   BSON_ASSERT (request);
   rpc = &request->request_rpc;

   if (request->opcode != MONGOC_OPCODE_GET_MORE) {
      test_error ("request's opcode does not match GET_MORE");
      return false;
   }

   if (strcmp (rpc->get_more.collection, ns)) {
      test_error ("request's namespace is '%s', expected '%s'",
                  request->request_rpc.get_more.collection,
                  ns);
      return false;
   }

   if (rpc->get_more.n_return != n_return) {
      test_error ("requests's n_return = %d, expected %d",
                  rpc->get_more.n_return,
                  n_return);
      return false;
   }

   if (rpc->get_more.cursor_id != cursor_id) {
      test_error ("requests's cursor_id = %" PRId64 ", expected %" PRId64,
                  rpc->get_more.cursor_id,
                  cursor_id);
      return false;
   }

   return true;
}


/* TODO: take file, line, function params from caller, wrap in macro */
bool
request_matches_kill_cursors (const request_t *request, int64_t cursor_id)
{
   const mongoc_rpc_t *rpc;

   BSON_ASSERT (request);
   rpc = &request->request_rpc;

   if (request->opcode != MONGOC_OPCODE_KILL_CURSORS) {
      test_error ("request's opcode does not match KILL_CURSORS");
      return false;
   }

   if (rpc->kill_cursors.n_cursors != 1) {
      test_error ("request's n_cursors is %d, expected 1",
                  rpc->kill_cursors.n_cursors);
      return false;
   }

   if (rpc->kill_cursors.cursors[0] != cursor_id) {
      test_error ("request's cursor_id %" PRId64 ", expected %" PRId64,
                  rpc->kill_cursors.cursors[0],
                  cursor_id);
      return false;
   }

   return true;
}


/*--------------------------------------------------------------------------
 *
 * request_get_server_port --
 *
 *       Get the port of the server this request was sent to.
 *
 * Returns:
 *       A port number.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

uint16_t
request_get_server_port (request_t *request)
{
   return mock_server_get_port (request->server);
}


/*--------------------------------------------------------------------------
 *
 * request_get_client_port --
 *
 *       Get the client port this request was sent from.
 *
 * Returns:
 *       A port number.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

uint16_t
request_get_client_port (request_t *request)
{
   return request->client_port;
}


/*--------------------------------------------------------------------------
 *
 * request_destroy --
 *
 *       Free a request_t.
 *
 * Returns:
 *       None.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

void
request_destroy (request_t *request)
{
   size_t i;
   bson_t *doc;

   for (i = 0; i < request->docs.len; i++) {
      doc = _mongoc_array_index (&request->docs, bson_t *, i);
      bson_destroy (doc);
   }

   _mongoc_array_destroy (&request->docs);
   bson_free (request->command_name);
   bson_free (request->as_str);
   bson_free (request->data);
   bson_free (request);
}


static bool
is_command_ns (const char *ns)
{
   size_t len = strlen (ns);
   const char *cmd = ".$cmd";
   size_t cmd_len = strlen (cmd);

   return len > cmd_len && !strncmp (ns + len - cmd_len, cmd, cmd_len);
}


static char *
query_flags_str (uint32_t flags)
{
   int flag = 1;
   bson_string_t *str = bson_string_new ("");
   bool begun = false;

   if (flags == MONGOC_QUERY_NONE) {
      bson_string_append (str, "0");
   } else {
      while (flag <= MONGOC_QUERY_PARTIAL) {
         flag <<= 1;

         if (flags & flag) {
            if (begun) {
               bson_string_append (str, "|");
            }

            begun = true;

            switch (flag) {
            case MONGOC_QUERY_TAILABLE_CURSOR:
               bson_string_append (str, "TAILABLE");
               break;
            case MONGOC_QUERY_SLAVE_OK:
               bson_string_append (str, "SLAVE_OK");
               break;
            case MONGOC_QUERY_OPLOG_REPLAY:
               bson_string_append (str, "OPLOG_REPLAY");
               break;
            case MONGOC_QUERY_NO_CURSOR_TIMEOUT:
               bson_string_append (str, "NO_TIMEOUT");
               break;
            case MONGOC_QUERY_AWAIT_DATA:
               bson_string_append (str, "AWAIT_DATA");
               break;
            case MONGOC_QUERY_EXHAUST:
               bson_string_append (str, "EXHAUST");
               break;
            case MONGOC_QUERY_PARTIAL:
               bson_string_append (str, "PARTIAL");
               break;
            case MONGOC_QUERY_NONE:
            default:
               BSON_ASSERT (false);
            }
         }
      }
   }

   return bson_string_free (str, false); /* detach buffer */
}


static void
request_from_query (request_t *request, const mongoc_rpc_t *rpc)
{
   int32_t len;
   bson_t *query;
   bson_t *fields;
   bson_iter_t iter;
   bson_string_t *query_as_str = bson_string_new ("OP_QUERY ");
   char *str;

   memcpy (&len, rpc->query.query, 4);
   len = BSON_UINT32_FROM_LE (len);
   query = bson_new_from_data (rpc->query.query, (size_t) len);
   BSON_ASSERT (query);
   _mongoc_array_append_val (&request->docs, query);

   bson_string_append_printf (query_as_str, "%s ", rpc->query.collection);

   if (is_command_ns (request->request_rpc.query.collection)) {
      request->is_command = true;

      if (bson_iter_init (&iter, query) && bson_iter_next (&iter)) {
         request->command_name = bson_strdup (bson_iter_key (&iter));
      } else {
         fprintf (stderr,
                  "WARNING: no command name for %s\n",
                  request->request_rpc.query.collection);
      }
   }

   str = bson_as_json (query, NULL);
   bson_string_append (query_as_str, str);
   bson_free (str);

   if (rpc->query.fields) {
      memcpy (&len, rpc->query.fields, 4);
      len = BSON_UINT32_FROM_LE (len);
      fields = bson_new_from_data (rpc->query.fields, (size_t) len);
      BSON_ASSERT (fields);
      _mongoc_array_append_val (&request->docs, fields);

      str = bson_as_json (fields, NULL);
      bson_string_append (query_as_str, " fields=");
      bson_string_append (query_as_str, str);
      bson_free (str);
   }

   bson_string_append (query_as_str, " flags=");

   str = query_flags_str (rpc->query.flags);
   bson_string_append (query_as_str, str);
   bson_free (str);

   if (rpc->query.skip) {
      bson_string_append_printf (
         query_as_str, " skip=%d", (int) rpc->query.skip);
   }

   if (rpc->query.n_return) {
      bson_string_append_printf (
         query_as_str, " n_return=%d", (int) rpc->query.n_return);
   }

   request->as_str = bson_string_free (query_as_str, false);
}


static char *
insert_flags_str (uint32_t flags)
{
   if (flags == MONGOC_INSERT_NONE) {
      return bson_strdup ("0");
   } else {
      return bson_strdup ("CONTINUE_ON_ERROR");
   }
}


static uint32_t
length_prefix (void *data)
{
   uint32_t len_le;

   memcpy (&len_le, data, sizeof (len_le));

   return BSON_UINT32_FROM_LE (len_le);
}


static void
request_from_insert (request_t *request, const mongoc_rpc_t *rpc)
{
   uint8_t *pos = (uint8_t *) request->request_rpc.insert.documents->iov_base;
   uint8_t *end = request->data + request->data_len;
   bson_string_t *insert_as_str = bson_string_new ("OP_INSERT");
   bson_t *doc;
   size_t n_documents;
   size_t i;
   char *str;

   while (pos < end) {
      uint32_t len = length_prefix (pos);
      doc = bson_new_from_data (pos, len);
      BSON_ASSERT (doc);
      _mongoc_array_append_val (&request->docs, doc);
      pos += len;
   }

   n_documents = request->docs.len;

   bson_string_append_printf (insert_as_str, " %d ", (int) n_documents);

   for (i = 0; i < n_documents; i++) {
      str = bson_as_json (request_get_doc (request, (int) i), NULL);
      BSON_ASSERT (str);
      bson_string_append (insert_as_str, str);
      bson_free (str);

      if (i < n_documents - 1) {
         bson_string_append (insert_as_str, ", ");
      }
   }

   bson_string_append (insert_as_str, " flags=");

   str = insert_flags_str (rpc->insert.flags);
   bson_string_append (insert_as_str, str);
   bson_free (str);

   request->as_str = bson_string_free (insert_as_str, false);
}


static char *
update_flags_str (uint32_t flags)
{
   int flag = 1;
   bson_string_t *str = bson_string_new ("");
   bool begun = false;

   if (flags == MONGOC_UPDATE_NONE) {
      bson_string_append (str, "0");
   } else {
      while (flag <= MONGOC_UPDATE_MULTI_UPDATE) {
         flag <<= 1;

         if (flags & flag) {
            if (begun) {
               bson_string_append (str, "|");
            }

            begun = true;

            switch (flag) {
            case MONGOC_UPDATE_UPSERT:
               bson_string_append (str, "UPSERT");
               break;
            case MONGOC_UPDATE_MULTI_UPDATE:
               bson_string_append (str, "MULTI");
               break;
            case MONGOC_UPDATE_NONE:
            default:
               BSON_ASSERT (false);
            }
         }
      }
   }

   return bson_string_free (str, false); /* detach buffer */
}


static void
request_from_update (request_t *request, const mongoc_rpc_t *rpc)
{
   int32_t len;
   bson_t *doc;
   bson_string_t *update_as_str = bson_string_new ("OP_UPDATE ");
   char *str;

   memcpy (&len, rpc->update.selector, 4);
   len = BSON_UINT32_FROM_LE (len);
   doc = bson_new_from_data (rpc->update.selector, (size_t) len);
   BSON_ASSERT (doc);
   _mongoc_array_append_val (&request->docs, doc);

   str = bson_as_json (doc, NULL);
   bson_string_append (update_as_str, str);
   bson_free (str);

   bson_string_append (update_as_str, ", ");

   memcpy (&len, rpc->update.update, 4);
   len = BSON_UINT32_FROM_LE (len);
   doc = bson_new_from_data (rpc->update.update, (size_t) len);
   BSON_ASSERT (doc);
   _mongoc_array_append_val (&request->docs, doc);

   str = bson_as_json (doc, NULL);
   bson_string_append (update_as_str, str);
   bson_free (str);

   bson_string_append (update_as_str, " flags=");

   str = update_flags_str (rpc->update.flags);
   bson_string_append (update_as_str, str);
   bson_free (str);

   request->as_str = bson_string_free (update_as_str, false);
}


static char *
delete_flags_str (uint32_t flags)
{
   if (flags == MONGOC_DELETE_NONE) {
      return bson_strdup ("0");
   } else {
      return bson_strdup ("SINGLE_REMOVE");
   }
}


static void
request_from_delete (request_t *request, const mongoc_rpc_t *rpc)
{
   int32_t len;
   bson_t *doc;
   bson_string_t *delete_as_str = bson_string_new ("OP_DELETE ");
   char *str;

   memcpy (&len, rpc->delete_.selector, 4);
   len = BSON_UINT32_FROM_LE (len);
   doc = bson_new_from_data (rpc->delete_.selector, (size_t) len);
   BSON_ASSERT (doc);
   _mongoc_array_append_val (&request->docs, doc);

   str = bson_as_json (doc, NULL);
   bson_string_append (delete_as_str, str);
   bson_free (str);

   bson_string_append (delete_as_str, " flags=");

   str = delete_flags_str (rpc->delete_.flags);
   bson_string_append (delete_as_str, str);
   bson_free (str);

   request->as_str = bson_string_free (delete_as_str, false);
}


static void
request_from_killcursors (request_t *request, const mongoc_rpc_t *rpc)
{
   /* protocol allows multiple cursor ids but we only implement one */
   BSON_ASSERT (rpc->kill_cursors.n_cursors == 1);
   request->as_str = bson_strdup_printf ("OP_KILLCURSORS %" PRId64,
                                         rpc->kill_cursors.cursors[0]);
}


static void
request_from_getmore (request_t *request, const mongoc_rpc_t *rpc)
{
   request->as_str =
      bson_strdup_printf ("OP_GETMORE %s %" PRId64 " n_return=%d",
                          rpc->get_more.collection,
                          rpc->get_more.cursor_id,
                          rpc->get_more.n_return);
}
