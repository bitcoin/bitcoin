#include <mongoc.h>
#include <mongoc-collection-private.h>
#include <mongoc-apm-private.h>
#include <mongoc-host-list-private.h>
#include <mongoc-cursor-private.h>
#include <mongoc-bulk-operation-private.h>
#include <mongoc-client-private.h>

#include "json-test.h"
#include "test-libmongoc.h"
#include "mock_server/mock-server.h"
#include "mock_server/future.h"
#include "mock_server/future-functions.h"


typedef struct {
   uint32_t n_events;
   bson_t events;
   mongoc_uri_t *test_framework_uri;
   int64_t cursor_id;
   int64_t operation_id;
   bool verbose;
} context_t;


static void
context_init (context_t *context)
{
   context->n_events = 0;
   bson_init (&context->events);
   context->test_framework_uri = test_framework_get_uri ();
   context->cursor_id = 0;
   context->operation_id = 0;
   context->verbose =
      test_framework_getenv_bool ("MONGOC_TEST_MONITORING_VERBOSE");
}


static void
context_destroy (context_t *context)
{
   bson_destroy (&context->events);
   mongoc_uri_destroy (context->test_framework_uri);
}


static int
check_server_version (const bson_t *test, context_t *context)
{
   const char *s;
   char *padded;
   server_version_t test_version, server_version;
   bool r;

   if (bson_has_field (test, "ignore_if_server_version_greater_than")) {
      s = bson_lookup_utf8 (test, "ignore_if_server_version_greater_than");
      /* s is like "3.0", don't skip if server is 3.0.x but skip 3.1+ */
      padded = bson_strdup_printf ("%s.99", s);
      test_version = test_framework_str_to_version (padded);
      bson_free (padded);
      server_version = test_framework_get_server_version ();
      r = server_version <= test_version;

      if (!r && context->verbose) {
         printf ("      SKIP, Server version > %s\n", s);
         fflush (stdout);
      }
   } else if (bson_has_field (test, "ignore_if_server_version_less_than")) {
      s = bson_lookup_utf8 (test, "ignore_if_server_version_less_than");
      test_version = test_framework_str_to_version (s);
      server_version = test_framework_get_server_version ();
      r = server_version >= test_version;

      if (!r && context->verbose) {
         printf ("      SKIP, Server version < %s\n", s);
         fflush (stdout);
      }
   } else {
      /* server version is ok, don't skip the test */
      return true;
   }

   return r;
}


static bool
check_topology_type (const bson_t *test)
{
   bson_iter_t iter;
   bson_iter_t child;
   bool is_mongos;
   const char *s;

   /* so far this field can only contain an array like ["sharded"],
    * see https://github.com/mongodb/specifications/pull/75
    */
   if (bson_iter_init_find (&iter, test, "ignore_if_topology_type")) {
      ASSERT (BSON_ITER_HOLDS_ARRAY (&iter));
      ASSERT (bson_iter_recurse (&iter, &child));

      is_mongos = test_framework_is_mongos ();

      while (bson_iter_next (&child)) {
         if (BSON_ITER_HOLDS_UTF8 (&child)) {
            s = bson_iter_utf8 (&child, NULL);

            /* skip test if topology type is sharded */
            if (!strcmp (s, "sharded") && is_mongos) {
               return false;
            }
         }
      }
   }

   return true;
}


static void
insert_data (mongoc_collection_t *collection, const bson_t *test)
{
   mongoc_bulk_operation_t *bulk;
   bson_iter_t iter;
   bson_iter_t array_iter;
   bson_t doc;
   uint32_t r;
   bson_error_t error;

   if (!mongoc_collection_drop (collection, &error)) {
      if (strcmp (error.message, "ns not found")) {
         /* an error besides ns not found */
         ASSERT_OR_PRINT (false, error);
      }
   }

   bulk = mongoc_collection_create_bulk_operation (collection, true, NULL);

   BSON_ASSERT (bson_iter_init_find (&iter, test, "data"));
   BSON_ASSERT (BSON_ITER_HOLDS_ARRAY (&iter));
   bson_iter_recurse (&iter, &array_iter);

   while (bson_iter_next (&array_iter)) {
      BSON_ASSERT (BSON_ITER_HOLDS_DOCUMENT (&array_iter));
      bson_iter_bson (&array_iter, &doc);
      mongoc_bulk_operation_insert (bulk, &doc);
      bson_destroy (&doc);
   }

   r = mongoc_bulk_operation_execute (bulk, NULL, &error);
   ASSERT_OR_PRINT (r, error);

   mongoc_bulk_operation_destroy (bulk);
}


static void
assert_host_in_uri (const mongoc_host_list_t *host, const mongoc_uri_t *uri)
{
   const mongoc_host_list_t *hosts;

   hosts = mongoc_uri_get_hosts (uri);
   while (hosts) {
      if (_mongoc_host_list_equal (hosts, host)) {
         return;
      }

      hosts = hosts->next;
   }

   fprintf (stderr,
            "Host \"%s\" not in \"%s\"",
            host->host_and_port,
            mongoc_uri_get_string (uri));
   fflush (stderr);
   abort ();
}


static bool
ends_with (const char *s, const char *suffix)
{
   size_t s_len;
   size_t suffix_len;

   if (!s) {
      return false;
   }

   s_len = strlen (s);
   suffix_len = strlen (suffix);
   return s_len >= suffix_len && !strcmp (s + s_len - suffix_len, suffix);
}


static int64_t
fake_cursor_id (const bson_iter_t *iter)
{
   return bson_iter_as_int64 (iter) ? 42 : 0;
}

/* Convert "ok" values to doubles, cursor ids and error codes to 42, and
 * error messages to "". See README at
 * github.com/mongodb/specifications/tree/master/source/command-monitoring/tests
 */
static void
convert_command_for_test (context_t *context,
                          const bson_t *src,
                          bson_t *dst,
                          const char *path)
{
   bson_iter_t iter;
   const char *key;
   const char *errmsg;
   bson_t src_child;
   bson_t dst_child;
   char *child_path;

   bson_iter_init (&iter, src);

   while (bson_iter_next (&iter)) {
      key = bson_iter_key (&iter);

      if (!strcmp (key, "ok")) {
         /* "The server is inconsistent on whether the ok values returned are
          * integers or doubles so for simplicity the tests specify all expected
          * values as doubles. Server 'ok' values of integers MUST be converted
          * to doubles for comparison with the expected values."
          */
         BSON_APPEND_DOUBLE (dst, key, (double) bson_iter_as_int64 (&iter));

      } else if (!strcmp (key, "errmsg")) {
         /* "errmsg values of "" MUST BSON_ASSERT that the value is not empty"
          */
         errmsg = bson_iter_utf8 (&iter, NULL);
         ASSERT_CMPSIZE_T (strlen (errmsg), >, (size_t) 0);
         BSON_APPEND_UTF8 (dst, key, "");

      } else if (!strcmp (key, "id") && ends_with (path, "cursor")) {
         /* "When encountering a cursor or getMore value of "42" in a test, the
          * driver MUST BSON_ASSERT that the values are equal to each other and
          * greater than zero."
          */
         if (context->cursor_id == 0) {
            context->cursor_id = bson_iter_int64 (&iter);
         } else if (bson_iter_int64 (&iter) != 0) {
            ASSERT_CMPINT64 (context->cursor_id, ==, bson_iter_int64 (&iter));
         }

         /* replace the reply's cursor id with 42 or 0 - check_expectations()
          * will BSON_ASSERT it matches the value from the JSON test */
         BSON_APPEND_INT64 (dst, key, fake_cursor_id (&iter));
      } else if (ends_with (path, "cursors") ||
                 ends_with (path, "cursorsUnknown")) {
         /* payload of a killCursors command-started event:
          *    {killCursors: "test", cursors: [12345]}
          * or killCursors command-succeeded event:
          *    {ok: 1, cursorsUnknown: [12345]}
          * */
         ASSERT_CMPINT64 (bson_iter_as_int64 (&iter), >, (int64_t) 0);
         BSON_APPEND_INT64 (dst, key, 42);

      } else if (!strcmp (key, "getMore")) {
         ASSERT_CMPINT64 (context->cursor_id, ==, bson_iter_int64 (&iter));
         BSON_APPEND_INT64 (dst, key, fake_cursor_id (&iter));

      } else if (!strcmp (key, "code")) {
         /* "code values of 42 MUST BSON_ASSERT that the value is present and
          * greater
          * than zero" */
         ASSERT_CMPINT64 (bson_iter_as_int64 (&iter), >, (int64_t) 0);
         BSON_APPEND_INT32 (dst, key, 42);

      } else if (BSON_ITER_HOLDS_DOCUMENT (&iter)) {
         if (path) {
            child_path = bson_strdup_printf ("%s.%s", path, key);
         } else {
            child_path = bson_strdup (key);
         }

         bson_iter_bson (&iter, &src_child);
         bson_append_document_begin (dst, key, -1, &dst_child);
         convert_command_for_test (
            context, &src_child, &dst_child, child_path); /* recurse */
         bson_append_document_end (dst, &dst_child);
         bson_free (child_path);
      } else if (BSON_ITER_HOLDS_ARRAY (&iter)) {
         if (path) {
            child_path = bson_strdup_printf ("%s.%s", path, key);
         } else {
            child_path = bson_strdup (key);
         }

         bson_iter_bson (&iter, &src_child);
         bson_append_array_begin (dst, key, -1, &dst_child);
         convert_command_for_test (
            context, &src_child, &dst_child, child_path); /* recurse */
         bson_append_array_end (dst, &dst_child);
         bson_free (child_path);
      } else {
         bson_append_value (dst, key, -1, bson_iter_value (&iter));
      }
   }
}


static void
started_cb (const mongoc_apm_command_started_t *event)
{
   context_t *context =
      (context_t *) mongoc_apm_command_started_get_context (event);
   int64_t operation_id;
   char *cmd_json;
   bson_t *events = &context->events;
   bson_t cmd = BSON_INITIALIZER;
   char str[16];
   const char *key;
   bson_t *new_event;

   if (context->verbose) {
      cmd_json = bson_as_canonical_extended_json (event->command, NULL);
      printf ("%s\n", cmd_json);
      fflush (stdout);
      bson_free (cmd_json);
   }

   BSON_ASSERT (mongoc_apm_command_started_get_request_id (event) > 0);
   BSON_ASSERT (mongoc_apm_command_started_get_server_id (event) > 0);
   assert_host_in_uri (event->host, context->test_framework_uri);

   /* subsequent events share the first event's operation id */
   operation_id = mongoc_apm_command_started_get_operation_id (event);
   ASSERT_CMPINT64 (operation_id, !=, (int64_t) 0);
   if (!context->operation_id) {
      context->operation_id = operation_id;
   } else {
      ASSERT_CMPINT64 (context->operation_id, ==, operation_id);
   }

   convert_command_for_test (context, event->command, &cmd, NULL);
   new_event = BCON_NEW ("command_started_event",
                         "{",
                         "command",
                         BCON_DOCUMENT (&cmd),
                         "command_name",
                         BCON_UTF8 (event->command_name),
                         "database_name",
                         BCON_UTF8 (event->database_name),
                         "}");

   bson_uint32_to_string (context->n_events, &key, str, sizeof str);
   BSON_APPEND_DOCUMENT (events, key, new_event);

   context->n_events++;

   bson_destroy (new_event);
   bson_destroy (&cmd);
}


static void
succeeded_cb (const mongoc_apm_command_succeeded_t *event)
{
   context_t *context =
      (context_t *) mongoc_apm_command_succeeded_get_context (event);
   int64_t operation_id;
   char *reply_json;
   bson_t reply = BSON_INITIALIZER;
   char str[16];
   const char *key;
   bson_t *new_event;

   if (context->verbose) {
      reply_json = bson_as_canonical_extended_json (event->reply, NULL);
      printf ("\t\t<-- %s\n", reply_json);
      fflush (stdout);
      bson_free (reply_json);
   }

   BSON_ASSERT (mongoc_apm_command_succeeded_get_request_id (event) > 0);
   BSON_ASSERT (mongoc_apm_command_succeeded_get_server_id (event) > 0);
   assert_host_in_uri (event->host, context->test_framework_uri);

   /* subsequent events share the first event's operation id */
   operation_id = mongoc_apm_command_succeeded_get_operation_id (event);
   ASSERT_CMPINT64 (operation_id, !=, (int64_t) 0);
   ASSERT_CMPINT64 (context->operation_id, ==, operation_id);

   convert_command_for_test (context, event->reply, &reply, NULL);
   new_event = BCON_NEW ("command_succeeded_event",
                         "{",
                         "reply",
                         BCON_DOCUMENT (&reply),
                         "command_name",
                         BCON_UTF8 (event->command_name),
                         "}");

   bson_uint32_to_string (context->n_events, &key, str, sizeof str);
   BSON_APPEND_DOCUMENT (&context->events, key, new_event);

   context->n_events++;

   bson_destroy (new_event);
   bson_destroy (&reply);
}


static void
failed_cb (const mongoc_apm_command_failed_t *event)
{
   context_t *context =
      (context_t *) mongoc_apm_command_failed_get_context (event);
   int64_t operation_id;
   bson_t reply = BSON_INITIALIZER;
   char str[16];
   const char *key;
   bson_t *new_event;

   if (context->verbose) {
      printf (
         "\t\t<-- %s FAILED: %s\n", event->command_name, event->error->message);
      fflush (stdout);
   }

   BSON_ASSERT (mongoc_apm_command_failed_get_request_id (event) > 0);
   BSON_ASSERT (mongoc_apm_command_failed_get_server_id (event) > 0);
   assert_host_in_uri (event->host, context->test_framework_uri);

   /* subsequent events share the first event's operation id */
   operation_id = mongoc_apm_command_failed_get_operation_id (event);
   ASSERT_CMPINT64 (operation_id, !=, (int64_t) 0);
   ASSERT_CMPINT64 (context->operation_id, ==, operation_id);

   new_event = BCON_NEW ("command_failed_event",
                         "{",
                         "command_name",
                         BCON_UTF8 (event->command_name),
                         "}");

   bson_uint32_to_string (context->n_events, &key, str, sizeof str);
   BSON_APPEND_DOCUMENT (&context->events, key, new_event);

   context->n_events++;

   bson_destroy (new_event);
   bson_destroy (&reply);
}


static void
one_bulk_op (mongoc_bulk_operation_t *bulk, const bson_t *request)
{
   bson_iter_t iter;
   const char *request_name;
   bson_t request_doc, document, filter, update;

   bson_iter_init (&iter, request);
   bson_iter_next (&iter);
   request_name = bson_iter_key (&iter);
   bson_iter_bson (&iter, &request_doc);

   if (!strcmp (request_name, "insertOne")) {
      bson_lookup_doc (&request_doc, "document", &document);
      mongoc_bulk_operation_insert (bulk, &document);
   } else if (!strcmp (request_name, "updateOne")) {
      bson_lookup_doc (&request_doc, "filter", &filter);
      bson_lookup_doc (&request_doc, "update", &update);
      mongoc_bulk_operation_update_one (
         bulk, &filter, &update, false /* upsert */);
   } else {
      test_error ("unrecognized request name %s", request_name);
      abort ();
   }
}


static void
test_bulk_write (mongoc_collection_t *collection, const bson_t *arguments)
{
   bool ordered;
   mongoc_write_concern_t *wc;
   mongoc_bulk_operation_t *bulk;
   bson_iter_t requests_iter;
   bson_t requests;
   bson_t request;
   uint32_t r;
   bson_error_t error;

   ordered = bson_lookup_bool (arguments, "ordered", true);

   if (bson_has_field (arguments, "writeConcern")) {
      wc = bson_lookup_write_concern (arguments, "writeConcern");
   } else {
      wc = mongoc_write_concern_new ();
   }

   if (bson_has_field (arguments, "requests")) {
      bson_lookup_doc (arguments, "requests", &requests);
   }

   bulk = mongoc_collection_create_bulk_operation (collection, ordered, wc);
   bson_iter_init (&requests_iter, &requests);
   while (bson_iter_next (&requests_iter)) {
      bson_iter_bson (&requests_iter, &request);
      one_bulk_op (bulk, &request);
   }

   r = mongoc_bulk_operation_execute (bulk, NULL, &error);
   ASSERT_OR_PRINT (r, error);

   mongoc_bulk_operation_destroy (bulk);
   mongoc_write_concern_destroy (wc);
}


static void
test_count (mongoc_collection_t *collection, const bson_t *arguments)
{
   bson_t filter;

   bson_lookup_doc (arguments, "filter", &filter);
   mongoc_collection_count (
      collection, MONGOC_QUERY_NONE, &filter, 0, 0, NULL, NULL);
}


static void
test_find (mongoc_collection_t *collection,
           const bson_t *arguments,
           mongoc_read_prefs_t *read_prefs)
{
   bson_t query;
   bson_t filter;
   bson_t sort;
   uint32_t skip = 0;
   uint32_t limit = 0;
   uint32_t batch_size = 0;
   bson_t modifiers;
   mongoc_cursor_t *cursor;
   const bson_t *doc;

   bson_lookup_doc (arguments, "filter", &filter);

   if (read_prefs || bson_has_field (arguments, "sort") ||
       bson_has_field (arguments, "modifiers")) {
      bson_init (&query);
      BSON_APPEND_DOCUMENT (&query, "$query", &filter);

      if (bson_has_field (arguments, "sort")) {
         bson_lookup_doc (arguments, "sort", &sort);
         BSON_APPEND_DOCUMENT (&query, "$orderby", &sort);
      }

      if (bson_has_field (arguments, "modifiers")) {
         bson_lookup_doc (arguments, "modifiers", &modifiers);
         bson_concat (&query, &modifiers);
      }
   } else {
      bson_copy_to (&filter, &query);
   }

   if (bson_has_field (arguments, "skip")) {
      skip = (uint32_t) bson_lookup_int64 (arguments, "skip");
   }

   if (bson_has_field (arguments, "limit")) {
      limit = (uint32_t) bson_lookup_int64 (arguments, "limit");
   }

   if (bson_has_field (arguments, "batchSize")) {
      batch_size = (uint32_t) bson_lookup_int64 (arguments, "batchSize");
   }

   cursor = mongoc_collection_find (collection,
                                    MONGOC_QUERY_NONE,
                                    skip,
                                    limit,
                                    batch_size,
                                    &query,
                                    NULL,
                                    read_prefs);

   BSON_ASSERT (cursor);
   while (mongoc_cursor_next (cursor, &doc)) {
   }

   /* can cause a killCursors command */
   mongoc_cursor_destroy (cursor);
   bson_destroy (&query);
}


static void
test_delete_many (mongoc_collection_t *collection, const bson_t *arguments)
{
   bson_t filter;

   bson_lookup_doc (arguments, "filter", &filter);
   mongoc_collection_remove (
      collection, MONGOC_REMOVE_NONE, &filter, NULL, NULL);
}


static void
test_delete_one (mongoc_collection_t *collection, const bson_t *arguments)
{
   bson_t filter;

   bson_lookup_doc (arguments, "filter", &filter);
   mongoc_collection_remove (
      collection, MONGOC_REMOVE_SINGLE_REMOVE, &filter, NULL, NULL);
}


static void
test_insert_many (mongoc_collection_t *collection, const bson_t *arguments)
{
   bool ordered;
   mongoc_bulk_operation_t *bulk;
   bson_t documents;
   bson_iter_t iter;
   bson_t doc;

   ordered = bson_lookup_bool (arguments, "ordered", true);
   bulk = mongoc_collection_create_bulk_operation (collection, ordered, NULL);

   bson_lookup_doc (arguments, "documents", &documents);
   bson_iter_init (&iter, &documents);
   while (bson_iter_next (&iter)) {
      bson_iter_bson (&iter, &doc);
      mongoc_bulk_operation_insert (bulk, &doc);
   }

   mongoc_bulk_operation_execute (bulk, NULL, NULL);

   mongoc_bulk_operation_destroy (bulk);
}


static void
test_insert_one (mongoc_collection_t *collection, const bson_t *arguments)
{
   bson_t document;

   bson_lookup_doc (arguments, "document", &document);
   mongoc_collection_insert (
      collection, MONGOC_INSERT_NONE, &document, NULL, NULL);
}


static void
test_update (mongoc_collection_t *collection,
             const bson_t *arguments,
             bool multi)
{
   bson_t filter;
   bson_t update;
   mongoc_update_flags_t flags = MONGOC_UPDATE_NONE;

   if (multi) {
      flags |= MONGOC_UPDATE_MULTI_UPDATE;
   }

   if (bson_lookup_bool (arguments, "upsert", false)) {
      flags |= MONGOC_UPDATE_UPSERT;
   }

   bson_lookup_doc (arguments, "filter", &filter);
   bson_lookup_doc (arguments, "update", &update);

   mongoc_collection_update (collection, flags, &filter, &update, NULL, NULL);
}


static void
test_update_many (mongoc_collection_t *collection, const bson_t *arguments)
{
   test_update (collection, arguments, true);
}


static void
test_update_one (mongoc_collection_t *collection, const bson_t *arguments)
{
   test_update (collection, arguments, false);
}


static void
one_test (mongoc_collection_t *collection, bson_t *test)
{
   context_t context;
   const char *description;
   mongoc_apm_callbacks_t *callbacks;
   bson_t operation;
   bson_t arguments;
   const char *op_name;
   mongoc_read_prefs_t *read_prefs = NULL;
   bson_t expectations;

   context_init (&context);
   callbacks = mongoc_apm_callbacks_new ();

   if (test_suite_debug_output ()) {
      description = bson_lookup_utf8 (test, "description");
      printf ("  - %s\n", description);
      fflush (stdout);
   }

   if (!check_server_version (test, &context) || !check_topology_type (test)) {
      goto done;
   }

   mongoc_apm_set_command_started_cb (callbacks, started_cb);
   mongoc_apm_set_command_succeeded_cb (callbacks, succeeded_cb);
   mongoc_apm_set_command_failed_cb (callbacks, failed_cb);
   mongoc_client_set_apm_callbacks (collection->client, callbacks, &context);

   bson_lookup_doc (test, "operation", &operation);
   op_name = bson_lookup_utf8 (&operation, "name");
   bson_lookup_doc (&operation, "arguments", &arguments);

   if (bson_has_field (&operation, "read_preference")) {
      read_prefs = bson_lookup_read_prefs (&operation, "read_preference");
   }

   if (!strcmp (op_name, "bulkWrite")) {
      test_bulk_write (collection, &arguments);
   } else if (!strcmp (op_name, "count")) {
      test_count (collection, &arguments);
   } else if (!strcmp (op_name, "find")) {
      test_find (collection, &arguments, read_prefs);
   } else if (!strcmp (op_name, "deleteMany")) {
      test_delete_many (collection, &arguments);
   } else if (!strcmp (op_name, "deleteOne")) {
      test_delete_one (collection, &arguments);
   } else if (!strcmp (op_name, "insertMany")) {
      test_insert_many (collection, &arguments);
   } else if (!strcmp (op_name, "insertOne")) {
      test_insert_one (collection, &arguments);
   } else if (!strcmp (op_name, "updateMany")) {
      test_update_many (collection, &arguments);
   } else if (!strcmp (op_name, "updateOne")) {
      test_update_one (collection, &arguments);
   } else {
      test_error ("unrecognized operation name %s", op_name);
      abort ();
   }

   bson_lookup_doc (test, "expectations", &expectations);
   check_json_apm_events (&context.events, &expectations);

done:
   mongoc_apm_callbacks_destroy (callbacks);
   context_destroy (&context);
   mongoc_read_prefs_destroy (read_prefs);
}


/*
 *-----------------------------------------------------------------------
 *
 * test_command_monitoring_cb --
 *
 *       Runs the JSON tests included with the Command Monitoring spec.
 *
 *-----------------------------------------------------------------------
 */

static void
test_command_monitoring_cb (bson_t *scenario)
{
   mongoc_client_t *client;
   const char *db_name;
   const char *collection_name;
   bson_iter_t iter;
   bson_iter_t tests_iter;
   bson_t test_op;
   mongoc_collection_t *collection;

   BSON_ASSERT (scenario);

   db_name = bson_lookup_utf8 (scenario, "database_name");
   collection_name = bson_lookup_utf8 (scenario, "collection_name");

   BSON_ASSERT (bson_iter_init_find (&iter, scenario, "tests"));
   BSON_ASSERT (BSON_ITER_HOLDS_ARRAY (&iter));
   bson_iter_recurse (&iter, &tests_iter);

   while (bson_iter_next (&tests_iter)) {
      client = test_framework_client_new ();
      collection =
         mongoc_client_get_collection (client, db_name, collection_name);

      insert_data (collection, scenario);
      bson_iter_bson (&tests_iter, &test_op);
      one_test (collection, &test_op);
      mongoc_collection_destroy (collection);
      mongoc_client_destroy (client);
   }
}


/*
 *-----------------------------------------------------------------------
 *
 * Runner for the JSON tests for command monitoring.
 *
 *-----------------------------------------------------------------------
 */
static void
test_all_spec_tests (TestSuite *suite)
{
   char resolved[PATH_MAX];

   ASSERT (realpath (JSON_DIR "/command_monitoring", resolved));
   install_json_test_suite (suite, resolved, &test_command_monitoring_cb);
}


static void
test_get_error_failed_cb (const mongoc_apm_command_failed_t *event)
{
   bson_error_t *error;

   error = (bson_error_t *) mongoc_apm_command_failed_get_context (event);
   mongoc_apm_command_failed_get_error (event, error);
}


static void
test_get_error (void)
{
   mock_server_t *server;
   mongoc_client_t *client;
   mongoc_apm_callbacks_t *callbacks;
   future_t *future;
   request_t *request;
   bson_error_t error = {0};

   server = mock_server_with_autoismaster (0);
   mock_server_run (server);

   client = mongoc_client_new_from_uri (mock_server_get_uri (server));
   callbacks = mongoc_apm_callbacks_new ();
   mongoc_apm_set_command_failed_cb (callbacks, test_get_error_failed_cb);
   mongoc_client_set_apm_callbacks (client, callbacks, (void *) &error);
   future = future_client_command_simple (
      client, "db", tmp_bson ("{'foo': 1}"), NULL, NULL, NULL);
   request = mock_server_receives_command (
      server, "db", MONGOC_QUERY_SLAVE_OK, "{'foo': 1}");
   mock_server_replies_simple (request,
                               "{'ok': 0, 'errmsg': 'foo', 'code': 42}");
   ASSERT (!future_get_bool (future));
   ASSERT_ERROR_CONTAINS (error, MONGOC_ERROR_QUERY, 42, "foo");

   future_destroy (future);
   request_destroy (request);
   mongoc_apm_callbacks_destroy (callbacks);
   mongoc_client_destroy (client);
   mock_server_destroy (server);
}


static void
insert_200_docs (mongoc_collection_t *collection)
{
   int i;
   bson_t *doc;
   bool r;
   bson_error_t error;

   /* insert 200 docs so we have a couple batches */
   doc = tmp_bson (NULL);
   for (i = 0; i < 200; i++) {
      r = mongoc_collection_insert (
         collection, MONGOC_INSERT_NONE, doc, NULL, &error);

      ASSERT_OR_PRINT (r, error);
   }
}


static void
increment (const mongoc_apm_command_started_t *event)
{
   int *i = (int *) mongoc_apm_command_started_get_context (event);

   ++(*i);
}


static mongoc_apm_callbacks_t *
increment_callbacks (void)
{
   mongoc_apm_callbacks_t *callbacks;

   callbacks = mongoc_apm_callbacks_new ();
   mongoc_apm_set_command_started_cb (callbacks, increment);

   return callbacks;
}


static void
decrement (const mongoc_apm_command_started_t *event)
{
   int *i = (int *) mongoc_apm_command_started_get_context (event);

   --(*i);
}


static mongoc_apm_callbacks_t *
decrement_callbacks (void)
{
   mongoc_apm_callbacks_t *callbacks;

   callbacks = mongoc_apm_callbacks_new ();
   mongoc_apm_set_command_started_cb (callbacks, decrement);

   return callbacks;
}


static void
test_change_callbacks (void *ctx)
{
   mongoc_apm_callbacks_t *inc_callbacks;
   mongoc_apm_callbacks_t *dec_callbacks;
   int incremented = 0;
   int decremented = 0;
   mongoc_client_t *client;
   mongoc_collection_t *collection;
   bson_error_t error;
   mongoc_cursor_t *cursor;
   const bson_t *b;

   inc_callbacks = increment_callbacks ();
   dec_callbacks = decrement_callbacks ();

   client = test_framework_client_new ();
   mongoc_client_set_apm_callbacks (client, inc_callbacks, &incremented);

   collection = get_test_collection (client, "test_change_callbacks");

   insert_200_docs (collection);
   ASSERT_CMPINT (incremented, ==, 200);

   mongoc_client_set_apm_callbacks (client, dec_callbacks, &decremented);
   cursor = mongoc_collection_aggregate (
      collection, MONGOC_QUERY_NONE, tmp_bson (NULL), NULL, NULL);

   ASSERT (mongoc_cursor_next (cursor, &b));
   ASSERT_CMPINT (decremented, ==, -1);

   mongoc_client_set_apm_callbacks (client, inc_callbacks, &incremented);
   while (mongoc_cursor_next (cursor, &b)) {
   }
   ASSERT_OR_PRINT (!mongoc_cursor_error (cursor, &error), error);
   ASSERT_CMPINT (incremented, ==, 201);

   mongoc_collection_drop (collection, NULL);

   mongoc_cursor_destroy (cursor);
   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
   mongoc_apm_callbacks_destroy (inc_callbacks);
   mongoc_apm_callbacks_destroy (dec_callbacks);
}


static void
test_reset_callbacks (void *ctx)
{
   mongoc_apm_callbacks_t *inc_callbacks;
   mongoc_apm_callbacks_t *dec_callbacks;
   int incremented = 0;
   int decremented = 0;
   mongoc_client_t *client;
   mongoc_collection_t *collection;
   bool r;
   bson_t *cmd;
   bson_t cmd_reply;
   bson_error_t error;
   mongoc_server_description_t *sd;
   mongoc_cursor_t *cursor;
   const bson_t *b;

   inc_callbacks = increment_callbacks ();
   dec_callbacks = decrement_callbacks ();

   client = test_framework_client_new ();
   collection = get_test_collection (client, "test_reset_apm_callbacks");

   /* insert 200 docs so we have a couple batches */
   insert_200_docs (collection);

   mongoc_client_set_apm_callbacks (client, inc_callbacks, &incremented);
   cmd = tmp_bson ("{'aggregate': '%s', 'pipeline': [], 'cursor': {}}",
                   collection->collection);

   sd =
      mongoc_client_select_server (client, true /* for writes */, NULL, &error);
   ASSERT_OR_PRINT (sd, error);

   r = mongoc_client_read_command_with_opts (
      client,
      "test",
      cmd,
      NULL,
      tmp_bson ("{'serverId': %d}", sd->id),
      &cmd_reply,
      &error);

   ASSERT_OR_PRINT (r, error);
   ASSERT_CMPINT (incremented, ==, 1);

   /* reset callbacks */
   mongoc_client_set_apm_callbacks (client, NULL, NULL);
   /* destroys cmd_reply */
   cursor = mongoc_cursor_new_from_command_reply (client, &cmd_reply, sd->id);
   ASSERT (mongoc_cursor_next (cursor, &b));
   ASSERT_CMPINT (incremented, ==, 1); /* same value as before */

   mongoc_client_set_apm_callbacks (client, dec_callbacks, &decremented);
   while (mongoc_cursor_next (cursor, &b)) {
   }
   ASSERT_OR_PRINT (!mongoc_cursor_error (cursor, &error), error);
   ASSERT_CMPINT (decremented, ==, -1);

   mongoc_collection_drop (collection, NULL);

   mongoc_cursor_destroy (cursor);
   mongoc_server_description_destroy (sd);
   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
   mongoc_apm_callbacks_destroy (inc_callbacks);
   mongoc_apm_callbacks_destroy (dec_callbacks);
}


static void
test_set_callbacks_cb (const mongoc_apm_command_started_t *event)
{
   int *n_calls = (int *) mongoc_apm_command_started_get_context (event);

   (*n_calls)++;
}


static void
_test_set_callbacks (bool pooled)
{
   mongoc_client_t *client;
   mongoc_client_pool_t *pool = NULL;
   mongoc_apm_callbacks_t *callbacks;
   int n_calls = 0;
   bson_error_t error;
   bson_t b;

   callbacks = mongoc_apm_callbacks_new ();
   mongoc_apm_set_command_started_cb (callbacks, test_set_callbacks_cb);

   if (pooled) {
      pool = test_framework_client_pool_new ();
      ASSERT (mongoc_client_pool_set_apm_callbacks (
         pool, callbacks, (void *) &n_calls));
      client = mongoc_client_pool_pop (pool);
   } else {
      client = test_framework_client_new ();
      ASSERT (mongoc_client_set_apm_callbacks (
         client, callbacks, (void *) &n_calls));
   }

   ASSERT_OR_PRINT (mongoc_client_get_server_status (client, NULL, &b, &error),
                    error);
   ASSERT_CMPINT (1, ==, n_calls);

   capture_logs (true);

   if (pooled) {
      ASSERT (
         !mongoc_client_pool_set_apm_callbacks (pool, NULL, (void *) &n_calls));
      ASSERT_CAPTURED_LOG ("mongoc_client_pool_set_apm_callbacks",
                           MONGOC_LOG_LEVEL_ERROR,
                           "Can only set callbacks once");

      clear_captured_logs ();
      ASSERT (
         !mongoc_client_set_apm_callbacks (client, NULL, (void *) &n_calls));
      ASSERT_CAPTURED_LOG ("mongoc_client_pool_set_apm_callbacks",
                           MONGOC_LOG_LEVEL_ERROR,
                           "Cannot set callbacks on a pooled client");
   } else {
      /* repeated calls ok, null is ok */
      ASSERT (mongoc_client_set_apm_callbacks (client, NULL, NULL));
   }

   if (pooled) {
      mongoc_client_pool_push (pool, client);
      mongoc_client_pool_destroy (pool);
   } else {
      mongoc_client_destroy (client);
   }

   bson_destroy (&b);
   mongoc_apm_callbacks_destroy (callbacks);
}


static void
test_set_callbacks_single (void)
{
   _test_set_callbacks (false);
}


static void
test_set_callbacks_pooled (void)
{
   _test_set_callbacks (true);
}


typedef struct {
   int64_t request_id;
   int64_t op_id;
} ids_t;


typedef struct {
   mongoc_array_t started_ids;
   mongoc_array_t succeeded_ids;
   mongoc_array_t failed_ids;
   int started_calls;
   int succeeded_calls;
   int failed_calls;
} op_id_test_t;


static void
op_id_test_init (op_id_test_t *test)
{
   _mongoc_array_init (&test->started_ids, sizeof (ids_t));
   _mongoc_array_init (&test->succeeded_ids, sizeof (ids_t));
   _mongoc_array_init (&test->failed_ids, sizeof (ids_t));

   test->started_calls = 0;
   test->succeeded_calls = 0;
   test->failed_calls = 0;
}


static void
op_id_test_cleanup (op_id_test_t *test)
{
   _mongoc_array_destroy (&test->started_ids);
   _mongoc_array_destroy (&test->succeeded_ids);
   _mongoc_array_destroy (&test->failed_ids);
}


static void
test_op_id_started_cb (const mongoc_apm_command_started_t *event)
{
   op_id_test_t *test;
   ids_t ids;

   test = (op_id_test_t *) mongoc_apm_command_started_get_context (event);
   ids.request_id = mongoc_apm_command_started_get_request_id (event);
   ids.op_id = mongoc_apm_command_started_get_operation_id (event);

   _mongoc_array_append_val (&test->started_ids, ids);

   test->started_calls++;
}


static void
test_op_id_succeeded_cb (const mongoc_apm_command_succeeded_t *event)
{
   op_id_test_t *test;
   ids_t ids;

   test = (op_id_test_t *) mongoc_apm_command_succeeded_get_context (event);
   ids.request_id = mongoc_apm_command_succeeded_get_request_id (event);
   ids.op_id = mongoc_apm_command_succeeded_get_operation_id (event);

   _mongoc_array_append_val (&test->succeeded_ids, ids);

   test->succeeded_calls++;
}


static void
test_op_id_failed_cb (const mongoc_apm_command_failed_t *event)
{
   op_id_test_t *test;
   ids_t ids;

   test = (op_id_test_t *) mongoc_apm_command_failed_get_context (event);
   ids.request_id = mongoc_apm_command_failed_get_request_id (event);
   ids.op_id = mongoc_apm_command_failed_get_operation_id (event);

   _mongoc_array_append_val (&test->failed_ids, ids);

   test->failed_calls++;
}


#define REQUEST_ID(_event_type, _index) \
   _mongoc_array_index (&test._event_type##_ids, ids_t, _index).request_id

#define OP_ID(_event_type, _index) \
   _mongoc_array_index (&test._event_type##_ids, ids_t, _index).op_id

static void
_test_bulk_operation_id (bool pooled, bool use_bulk_operation_new)
{
   mongoc_client_t *client;
   mongoc_client_pool_t *pool = NULL;
   mongoc_apm_callbacks_t *callbacks;
   mongoc_collection_t *collection;
   mongoc_bulk_operation_t *bulk;
   bson_error_t error;
   op_id_test_t test;
   int64_t op_id;

   op_id_test_init (&test);

   callbacks = mongoc_apm_callbacks_new ();
   mongoc_apm_set_command_started_cb (callbacks, test_op_id_started_cb);
   mongoc_apm_set_command_succeeded_cb (callbacks, test_op_id_succeeded_cb);
   mongoc_apm_set_command_failed_cb (callbacks, test_op_id_failed_cb);

   if (pooled) {
      pool = test_framework_client_pool_new ();
      ASSERT (mongoc_client_pool_set_apm_callbacks (
         pool, callbacks, (void *) &test));
      client = mongoc_client_pool_pop (pool);
   } else {
      client = test_framework_client_new ();
      ASSERT (
         mongoc_client_set_apm_callbacks (client, callbacks, (void *) &test));
   }

   collection = get_test_collection (client, "test_bulk_operation_id");
   if (use_bulk_operation_new) {
      bulk = mongoc_bulk_operation_new (false);
      mongoc_bulk_operation_set_client (bulk, client);
      mongoc_bulk_operation_set_database (bulk, collection->db);
      mongoc_bulk_operation_set_collection (bulk, collection->collection);
   } else {
      bulk = mongoc_collection_create_bulk_operation (collection, false, NULL);
   }

   mongoc_bulk_operation_insert (bulk, tmp_bson ("{'_id': 1}"));
   mongoc_bulk_operation_update_one (
      bulk, tmp_bson ("{'_id': 1}"), tmp_bson ("{'$set': {'x': 1}}"), false);
   mongoc_bulk_operation_remove (bulk, tmp_bson ("{}"));

   /* ensure we monitor with bulk->operation_id, not cluster->operation_id */
   client->cluster.operation_id = 42;

   /* write errors don't trigger failed events, so we only test success */
   ASSERT_OR_PRINT (mongoc_bulk_operation_execute (bulk, NULL, &error), error);
   ASSERT_CMPINT (test.started_calls, ==, 3);
   ASSERT_CMPINT (test.succeeded_calls, ==, 3);

   ASSERT_CMPINT64 (REQUEST_ID (started, 0), ==, REQUEST_ID (succeeded, 0));
   ASSERT_CMPINT64 (REQUEST_ID (started, 1), ==, REQUEST_ID (succeeded, 1));
   ASSERT_CMPINT64 (REQUEST_ID (started, 2), ==, REQUEST_ID (succeeded, 2));

   /* 3 unique request ids */
   ASSERT_CMPINT64 (REQUEST_ID (started, 0), !=, REQUEST_ID (started, 1));
   ASSERT_CMPINT64 (REQUEST_ID (started, 0), !=, REQUEST_ID (started, 2));
   ASSERT_CMPINT64 (REQUEST_ID (started, 1), !=, REQUEST_ID (started, 2));
   ASSERT_CMPINT64 (REQUEST_ID (succeeded, 0), !=, REQUEST_ID (succeeded, 1));
   ASSERT_CMPINT64 (REQUEST_ID (succeeded, 0), !=, REQUEST_ID (succeeded, 2));
   ASSERT_CMPINT64 (REQUEST_ID (succeeded, 1), !=, REQUEST_ID (succeeded, 2));

   /* events' operation ids all equal bulk->operation_id */
   op_id = bulk->operation_id;
   ASSERT_CMPINT64 (op_id, !=, (int64_t) 0);
   ASSERT_CMPINT64 (op_id, ==, OP_ID (started, 0));
   ASSERT_CMPINT64 (op_id, ==, OP_ID (started, 1));
   ASSERT_CMPINT64 (op_id, ==, OP_ID (started, 2));
   ASSERT_CMPINT64 (op_id, ==, OP_ID (succeeded, 0));
   ASSERT_CMPINT64 (op_id, ==, OP_ID (succeeded, 1));
   ASSERT_CMPINT64 (op_id, ==, OP_ID (succeeded, 2));

   mongoc_bulk_operation_destroy (bulk);
   mongoc_collection_destroy (collection);

   if (pooled) {
      mongoc_client_pool_push (pool, client);
      mongoc_client_pool_destroy (pool);
   } else {
      mongoc_client_destroy (client);
   }

   op_id_test_cleanup (&test);
   mongoc_apm_callbacks_destroy (callbacks);
}


static void
test_collection_bulk_op_single (void)
{
   _test_bulk_operation_id (false, false);
}


static void
test_collection_bulk_op_pooled (void)
{
   _test_bulk_operation_id (true, false);
}


static void
test_bulk_op_single (void)
{
   _test_bulk_operation_id (false, true);
}


static void
test_bulk_op_pooled (void)
{
   _test_bulk_operation_id (true, true);
}


static void
_test_query_operation_id (bool pooled, bool use_cmd)
{
   mock_server_t *server;
   mongoc_client_t *client;
   mongoc_client_pool_t *pool = NULL;
   mongoc_apm_callbacks_t *callbacks;
   mongoc_collection_t *collection;
   op_id_test_t test;
   mongoc_cursor_t *cursor;
   const bson_t *doc;
   future_t *future;
   request_t *request;
   int64_t op_id;

   op_id_test_init (&test);

   server = mock_server_with_autoismaster (use_cmd ? 4 : 0);
   mock_server_run (server);

   callbacks = mongoc_apm_callbacks_new ();
   mongoc_apm_set_command_started_cb (callbacks, test_op_id_started_cb);
   mongoc_apm_set_command_succeeded_cb (callbacks, test_op_id_succeeded_cb);
   mongoc_apm_set_command_failed_cb (callbacks, test_op_id_failed_cb);

   if (pooled) {
      pool = mongoc_client_pool_new (mock_server_get_uri (server));
      ASSERT (mongoc_client_pool_set_apm_callbacks (
         pool, callbacks, (void *) &test));
      client = mongoc_client_pool_pop (pool);
   } else {
      client = mongoc_client_new_from_uri (mock_server_get_uri (server));
      ASSERT (
         mongoc_client_set_apm_callbacks (client, callbacks, (void *) &test));
   }

   collection = mongoc_client_get_collection (client, "db", "collection");
   cursor = mongoc_collection_find (
      collection, MONGOC_QUERY_NONE, 0, 0, 1, tmp_bson ("{}"), NULL, NULL);

   future = future_cursor_next (cursor, &doc);
   request = mock_server_receives_request (server);
   mock_server_replies_to_find (request,
                                MONGOC_QUERY_SLAVE_OK,
                                123 /* cursor id */,
                                1,
                                "db.collection",
                                "{}",
                                use_cmd);

   ASSERT (future_get_bool (future));
   future_destroy (future);
   request_destroy (request);

   ASSERT_CMPINT (test.started_calls, ==, 1);
   ASSERT_CMPINT (test.succeeded_calls, ==, 1);

   future = future_cursor_next (cursor, &doc);
   request = mock_server_receives_request (server);
   if (use_cmd) {
      mock_server_replies_simple (request,
                                  "{'ok': 0, 'code': 42, 'errmsg': 'bad!'}");
   } else {
      mock_server_replies (request,
                           MONGOC_REPLY_QUERY_FAILURE,
                           123,
                           0,
                           0,
                           "{'$err': 'uh oh', 'code': 4321}");
   }

   ASSERT (!future_get_bool (future));
   future_destroy (future);
   request_destroy (request);

   ASSERT_CMPINT (test.started_calls, ==, 2);
   ASSERT_CMPINT (test.succeeded_calls, ==, 1);
   ASSERT_CMPINT (test.failed_calls, ==, 1);

   ASSERT_CMPINT64 (REQUEST_ID (started, 0), ==, REQUEST_ID (succeeded, 0));
   ASSERT_CMPINT64 (REQUEST_ID (started, 1), ==, REQUEST_ID (failed, 0));

   /* unique request ids */
   ASSERT_CMPINT64 (REQUEST_ID (started, 0), !=, REQUEST_ID (started, 1));

   /* operation ids all the same */
   op_id = OP_ID (started, 0);
   ASSERT_CMPINT64 (op_id, !=, (int64_t) 0);
   ASSERT_CMPINT64 (op_id, ==, OP_ID (started, 1));
   ASSERT_CMPINT64 (op_id, ==, OP_ID (failed, 0));

   mock_server_destroy (server);

   /* client logs warning because it can't send killCursors */
   capture_logs (true);
   mongoc_cursor_destroy (cursor);
   mongoc_collection_destroy (collection);

   if (pooled) {
      mongoc_client_pool_push (pool, client);
      mongoc_client_pool_destroy (pool);
   } else {
      mongoc_client_destroy (client);
   }

   op_id_test_cleanup (&test);
   mongoc_apm_callbacks_destroy (callbacks);
}


static void
test_query_operation_id_single_cmd (void)
{
   _test_query_operation_id (false, true);
}


static void
test_query_operation_id_pooled_cmd (void)
{
   _test_query_operation_id (true, true);
}


static void
test_query_operation_id_single_op_query (void)
{
   _test_query_operation_id (false, false);
}


static void
test_query_operation_id_pooled_op_query (void)
{
   _test_query_operation_id (true, false);
}


typedef struct {
   int started_calls;
   int succeeded_calls;
   int failed_calls;
   char db[100];
   char cmd_name[100];
   bson_t cmd;
} cmd_test_t;


static void
cmd_test_init (cmd_test_t *test)
{
   memset (test, 0, sizeof *test);
   bson_init (&test->cmd);
}


static void
cmd_test_cleanup (cmd_test_t *test)
{
   bson_destroy (&test->cmd);
}


static void
cmd_started_cb (const mongoc_apm_command_started_t *event)
{
   cmd_test_t *test;

   test = (cmd_test_t *) mongoc_apm_command_started_get_context (event);
   test->started_calls++;
   bson_destroy (&test->cmd);
   bson_strncpy (test->db,
                 mongoc_apm_command_started_get_database_name (event),
                 sizeof (test->db));
   bson_copy_to (mongoc_apm_command_started_get_command (event), &test->cmd);
   bson_strncpy (test->cmd_name,
                 mongoc_apm_command_started_get_command_name (event),
                 sizeof (test->cmd_name));
}


static void
cmd_succeeded_cb (const mongoc_apm_command_succeeded_t *event)
{
   cmd_test_t *test;

   test = (cmd_test_t *) mongoc_apm_command_succeeded_get_context (event);
   test->succeeded_calls++;
   ASSERT_CMPSTR (test->cmd_name,
                  mongoc_apm_command_succeeded_get_command_name (event));
}


static void
cmd_failed_cb (const mongoc_apm_command_failed_t *event)
{
   cmd_test_t *test;

   test = (cmd_test_t *) mongoc_apm_command_failed_get_context (event);
   test->failed_calls++;
   ASSERT_CMPSTR (test->cmd_name,
                  mongoc_apm_command_failed_get_command_name (event));
}


static void
set_cmd_test_callbacks (mongoc_client_t *client, void *context)
{
   mongoc_apm_callbacks_t *callbacks;

   callbacks = mongoc_apm_callbacks_new ();
   mongoc_apm_set_command_started_cb (callbacks, cmd_started_cb);
   mongoc_apm_set_command_succeeded_cb (callbacks, cmd_succeeded_cb);
   mongoc_apm_set_command_failed_cb (callbacks, cmd_failed_cb);
   ASSERT (mongoc_client_set_apm_callbacks (client, callbacks, context));
   mongoc_apm_callbacks_destroy (callbacks);
}


static void
test_client_cmd (void)
{
   cmd_test_t test;
   mongoc_client_t *client;
   mongoc_cursor_t *cursor;
   const bson_t *reply;

   cmd_test_init (&test);
   client = test_framework_client_new ();
   set_cmd_test_callbacks (client, (void *) &test);
   cursor = mongoc_client_command (client,
                                   "admin",
                                   MONGOC_QUERY_SLAVE_OK,
                                   0,
                                   0,
                                   0,
                                   tmp_bson ("{'ping': 1}"),
                                   NULL,
                                   NULL);

   ASSERT (mongoc_cursor_next (cursor, &reply));
   ASSERT_CMPSTR (test.cmd_name, "ping");
   ASSERT_MATCH (&test.cmd, "{'ping': 1}");
   ASSERT_CMPSTR (test.db, "admin");
   ASSERT_CMPINT (1, ==, test.started_calls);
   ASSERT_CMPINT (1, ==, test.succeeded_calls);
   ASSERT_CMPINT (0, ==, test.failed_calls);

   cmd_test_cleanup (&test);
   mongoc_cursor_destroy (cursor);

   cmd_test_init (&test);
   cursor = mongoc_client_command (client,
                                   "admin",
                                   MONGOC_QUERY_SLAVE_OK,
                                   0,
                                   0,
                                   0,
                                   tmp_bson ("{'foo': 1}"),
                                   NULL,
                                   NULL);

   ASSERT (!mongoc_cursor_next (cursor, &reply));
   ASSERT_CMPSTR (test.cmd_name, "foo");
   ASSERT_MATCH (&test.cmd, "{'foo': 1}");
   ASSERT_CMPSTR (test.db, "admin");
   ASSERT_CMPINT (1, ==, test.started_calls);
   ASSERT_CMPINT (0, ==, test.succeeded_calls);
   ASSERT_CMPINT (1, ==, test.failed_calls);

   cmd_test_cleanup (&test);
   mongoc_cursor_destroy (cursor);
   mongoc_client_destroy (client);
}


static void
test_client_cmd_simple (void)
{
   cmd_test_t test;
   mongoc_client_t *client;
   bool r;
   bson_error_t error;

   cmd_test_init (&test);
   client = test_framework_client_new ();
   set_cmd_test_callbacks (client, (void *) &test);
   r = mongoc_client_command_simple (
      client, "admin", tmp_bson ("{'ping': 1}"), NULL, NULL, &error);

   ASSERT_OR_PRINT (r, error);
   ASSERT_CMPSTR (test.cmd_name, "ping");
   ASSERT_MATCH (&test.cmd, "{'ping': 1}");
   ASSERT_CMPSTR (test.db, "admin");
   ASSERT_CMPINT (1, ==, test.started_calls);
   ASSERT_CMPINT (1, ==, test.succeeded_calls);
   ASSERT_CMPINT (0, ==, test.failed_calls);

   cmd_test_cleanup (&test);
   cmd_test_init (&test);
   r = mongoc_client_command_simple (
      client, "admin", tmp_bson ("{'foo': 1}"), NULL, NULL, &error);

   ASSERT (!r);
   ASSERT_CMPSTR (test.cmd_name, "foo");
   ASSERT_MATCH (&test.cmd, "{'foo': 1}");
   ASSERT_CMPSTR (test.db, "admin");
   ASSERT_CMPINT (1, ==, test.started_calls);
   ASSERT_CMPINT (0, ==, test.succeeded_calls);
   ASSERT_CMPINT (1, ==, test.failed_calls);

   mongoc_client_destroy (client);
   cmd_test_cleanup (&test);
}


static void
test_client_cmd_op_ids (void)
{
   op_id_test_t test;
   mongoc_client_t *client;
   mongoc_apm_callbacks_t *callbacks;
   bool r;
   bson_error_t error;
   int64_t op_id;

   op_id_test_init (&test);

   callbacks = mongoc_apm_callbacks_new ();
   mongoc_apm_set_command_started_cb (callbacks, test_op_id_started_cb);
   mongoc_apm_set_command_succeeded_cb (callbacks, test_op_id_succeeded_cb);
   mongoc_apm_set_command_failed_cb (callbacks, test_op_id_failed_cb);

   client = test_framework_client_new ();
   mongoc_client_set_apm_callbacks (client, callbacks, (void *) &test);

   r = mongoc_client_command_simple (
      client, "admin", tmp_bson ("{'ping': 1}"), NULL, NULL, &error);

   ASSERT_OR_PRINT (r, error);
   ASSERT_CMPINT (1, ==, test.started_calls);
   ASSERT_CMPINT (1, ==, test.succeeded_calls);
   ASSERT_CMPINT (0, ==, test.failed_calls);
   ASSERT_CMPINT64 (REQUEST_ID (started, 0), ==, REQUEST_ID (succeeded, 0));
   ASSERT_CMPINT64 (OP_ID (started, 0), ==, OP_ID (succeeded, 0));
   op_id = OP_ID (started, 0);
   ASSERT_CMPINT64 (op_id, !=, (int64_t) 0);

   op_id_test_cleanup (&test);
   op_id_test_init (&test);

   /* again. test that we use a new op_id. */
   r = mongoc_client_command_simple (
      client, "admin", tmp_bson ("{'ping': 1}"), NULL, NULL, &error);

   ASSERT_OR_PRINT (r, error);
   ASSERT_CMPINT (1, ==, test.started_calls);
   ASSERT_CMPINT (1, ==, test.succeeded_calls);
   ASSERT_CMPINT (0, ==, test.failed_calls);
   ASSERT_CMPINT64 (REQUEST_ID (started, 0), ==, REQUEST_ID (succeeded, 0));
   ASSERT_CMPINT64 (OP_ID (started, 0), ==, OP_ID (succeeded, 0));
   ASSERT_CMPINT64 (OP_ID (started, 0), !=, (int64_t) 0);

   /* new op_id */
   ASSERT_CMPINT64 (OP_ID (started, 0), !=, op_id);

   mongoc_client_destroy (client);
   op_id_test_cleanup (&test);
   mongoc_apm_callbacks_destroy (callbacks);
}


static void
test_killcursors_deprecated (void)
{
   cmd_test_t test;
   mongoc_client_t *client;
   bool r;
   bson_error_t error;

   cmd_test_init (&test);
   client = test_framework_client_new ();

   /* connect */
   r = mongoc_client_command_simple (
      client, "admin", tmp_bson ("{'ping': 1}"), NULL, NULL, &error);

   ASSERT_OR_PRINT (r, error);
   set_cmd_test_callbacks (client, (void *) &test);

   /* deprecated function without "db" or "collection", skips APM */
   mongoc_client_kill_cursor (client, 123);

   ASSERT_CMPINT (0, ==, test.started_calls);
   ASSERT_CMPINT (0, ==, test.succeeded_calls);
   ASSERT_CMPINT (0, ==, test.failed_calls);

   mongoc_client_destroy (client);
   cmd_test_cleanup (&test);
}


void
test_command_monitoring_install (TestSuite *suite)
{
   test_all_spec_tests (suite);
   TestSuite_AddMockServerTest (
      suite, "/command_monitoring/get_error", test_get_error);
   TestSuite_AddLive (suite,
                      "/command_monitoring/set_callbacks/single",
                      test_set_callbacks_single);
   TestSuite_AddLive (suite,
                      "/command_monitoring/set_callbacks/pooled",
                      test_set_callbacks_pooled);
   /* require aggregation cursor */
   TestSuite_AddFull (suite,
                      "/command_monitoring/set_callbacks/change",
                      test_change_callbacks,
                      NULL,
                      NULL,
                      test_framework_skip_if_max_wire_version_less_than_1);
   TestSuite_AddFull (suite,
                      "/command_monitoring/set_callbacks/reset",
                      test_reset_callbacks,
                      NULL,
                      NULL,
                      test_framework_skip_if_max_wire_version_less_than_1);
   TestSuite_AddLive (suite,
                      "/command_monitoring/operation_id/bulk/collection/single",
                      test_collection_bulk_op_single);
   TestSuite_AddLive (suite,
                      "/command_monitoring/operation_id/bulk/collection/pooled",
                      test_collection_bulk_op_pooled);
   TestSuite_AddLive (suite,
                      "/command_monitoring/operation_id/bulk/new/single",
                      test_bulk_op_single);
   TestSuite_AddLive (suite,
                      "/command_monitoring/operation_id/bulk/new/pooled",
                      test_bulk_op_pooled);
   TestSuite_AddMockServerTest (
      suite,
      "/command_monitoring/operation_id/query/single/cmd",
      test_query_operation_id_single_cmd);
   TestSuite_AddMockServerTest (
      suite,
      "/command_monitoring/operation_id/query/pooled/cmd",
      test_query_operation_id_pooled_cmd);
   TestSuite_AddMockServerTest (
      suite,
      "/command_monitoring/operation_id/query/single/op_query",
      test_query_operation_id_single_op_query);
   TestSuite_AddMockServerTest (
      suite,
      "/command_monitoring/operation_id/query/pooled/op_query",
      test_query_operation_id_pooled_op_query);
   TestSuite_AddLive (suite, "/command_monitoring/client_cmd", test_client_cmd);
   TestSuite_AddLive (
      suite, "/command_monitoring/client_cmd_simple", test_client_cmd_simple);
   TestSuite_AddLive (
      suite, "/command_monitoring/client_cmd/op_ids", test_client_cmd_op_ids);
   TestSuite_AddLive (suite,
                      "/command_monitoring/killcursors_deprecated",
                      test_killcursors_deprecated);
}
