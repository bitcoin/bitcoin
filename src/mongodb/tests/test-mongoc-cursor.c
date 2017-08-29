#include <mongoc.h>
#include <mongoc-client-private.h>

#include "TestSuite.h"
#include "test-libmongoc.h"
#include "mock_server/mock-rs.h"
#include "mock_server/future-functions.h"
#include "mongoc-cursor-private.h"
#include "mongoc-collection-private.h"
#include "test-conveniences.h"


static void
test_get_host (void)
{
   const mongoc_host_list_t *hosts;
   mongoc_host_list_t host;
   mongoc_client_t *client;
   mongoc_cursor_t *cursor;
   char *uri_str = test_framework_get_uri_str ();
   mongoc_uri_t *uri;
   const bson_t *doc;
   bson_error_t error;
   bool r;
   bson_t q = BSON_INITIALIZER;

   uri = mongoc_uri_new (uri_str);

   hosts = mongoc_uri_get_hosts (uri);

   client = test_framework_client_new ();
   cursor = _mongoc_cursor_new (client,
                                "test.test",
                                MONGOC_QUERY_NONE,
                                0,
                                1,
                                1,
                                false,
                                &q,
                                NULL,
                                NULL,
                                NULL);
   r = mongoc_cursor_next (cursor, &doc);
   if (!r && mongoc_cursor_error (cursor, &error)) {
      test_error ("%s", error.message);
      abort ();
   }

   BSON_ASSERT (doc == mongoc_cursor_current (cursor));

   mongoc_cursor_get_host (cursor, &host);

   /* In a production deployment the driver can discover servers not in the seed
    * list, but for this test assume the cursor uses one of the seeds. */
   while (hosts) {
      if (!strcmp (host.host_and_port, hosts->host_and_port)) {
         /* the cursor is using this server */
         ASSERT_CMPSTR (host.host, hosts->host);
         ASSERT_CMPINT (host.port, ==, hosts->port);
         ASSERT_CMPINT (host.family, ==, hosts->family);
         break;
      }

      hosts = hosts->next;
   }

   if (!hosts) {
      test_error (
         "cursor using host %s not in seeds: %s", host.host_and_port, uri_str);
      abort ();
   }

   bson_free (uri_str);
   mongoc_uri_destroy (uri);
   mongoc_cursor_destroy (cursor);
   mongoc_client_destroy (client);
}

static void
test_clone (void)
{
   mongoc_cursor_t *clone;
   mongoc_cursor_t *cursor;
   mongoc_client_t *client;
   const bson_t *doc;
   bson_error_t error;
   bool r;
   bson_t q = BSON_INITIALIZER;

   client = test_framework_client_new ();

   {
      /*
       * Ensure test.test has a document.
       */

      mongoc_collection_t *col;

      col = mongoc_client_get_collection (client, "test", "test");
      r = mongoc_collection_insert (col, MONGOC_INSERT_NONE, &q, NULL, &error);
      ASSERT (r);

      mongoc_collection_destroy (col);
   }

   cursor = _mongoc_cursor_new (client,
                                "test.test",
                                MONGOC_QUERY_NONE,
                                0,
                                1,
                                1,
                                false,
                                &q,
                                NULL,
                                NULL,
                                NULL);
   ASSERT (cursor);

   r = mongoc_cursor_next (cursor, &doc);
   if (!r || mongoc_cursor_error (cursor, &error)) {
      test_error ("%s", error.message);
      abort ();
   }
   ASSERT (doc);

   clone = mongoc_cursor_clone (cursor);
   ASSERT (cursor);

   r = mongoc_cursor_next (clone, &doc);
   if (!r || mongoc_cursor_error (clone, &error)) {
      test_error ("%s", error.message);
      abort ();
   }
   ASSERT (doc);

   mongoc_cursor_destroy (cursor);
   mongoc_cursor_destroy (clone);
   mongoc_client_destroy (client);
}


static void
test_limit (void)
{
   mongoc_client_t *client;
   mongoc_collection_t *collection;
   mongoc_bulk_operation_t *bulk;
   bson_t *b;
   bson_t *opts;
   int i, n_docs;
   mongoc_cursor_t *cursor;
   bson_error_t error;
   int64_t limits[] = {5, -5};
   const bson_t *doc = NULL;
   bool r;

   client = test_framework_client_new ();
   collection = get_test_collection (client, "test_limit");
   bulk = mongoc_collection_create_bulk_operation (collection, true, NULL);
   b = tmp_bson ("{}");
   for (i = 0; i < 10; ++i) {
      mongoc_bulk_operation_insert (bulk, b);
   }

   r = (0 != mongoc_bulk_operation_execute (bulk, NULL, &error));
   ASSERT_OR_PRINT (r, error);

   /* test positive and negative limit */
   for (i = 0; i < 2; i++) {
      cursor = mongoc_collection_find (
         collection, MONGOC_QUERY_NONE, 0, 0, 0, tmp_bson ("{}"), NULL, NULL);
      ASSERT_CMPINT64 ((int64_t) 0, ==, mongoc_cursor_get_limit (cursor));
      ASSERT (mongoc_cursor_set_limit (cursor, limits[i]));
      ASSERT_CMPINT64 (limits[i], ==, mongoc_cursor_get_limit (cursor));
      n_docs = 0;
      while (mongoc_cursor_next (cursor, &doc)) {
         ++n_docs;
      }

      ASSERT_OR_PRINT (!mongoc_cursor_error (cursor, &error), error);
      ASSERT (!mongoc_cursor_is_alive (cursor));
      ASSERT (!mongoc_cursor_more (cursor));
      ASSERT_CMPINT (n_docs, ==, 5);
      ASSERT (!mongoc_cursor_set_limit (cursor, 123)); /* no effect */
      ASSERT_CMPINT64 (limits[i], ==, mongoc_cursor_get_limit (cursor));

      mongoc_cursor_destroy (cursor);

      if (limits[i] > 0) {
         opts = tmp_bson ("{'limit': {'$numberLong': '%d'}}", limits[i]);
      } else {
         opts =
            tmp_bson ("{'singleBatch': true, 'limit': {'$numberLong': '%d'}}",
                      -limits[i]);
      }

      cursor = mongoc_collection_find_with_opts (
         collection, tmp_bson (NULL), opts, NULL);

      ASSERT_CMPINT64 (limits[i], ==, mongoc_cursor_get_limit (cursor));
      n_docs = 0;
      while (mongoc_cursor_next (cursor, &doc)) {
         ++n_docs;
      }

      ASSERT_OR_PRINT (!mongoc_cursor_error (cursor, &error), error);
      ASSERT_CMPINT (n_docs, ==, 5);

      mongoc_cursor_destroy (cursor);
   }

   mongoc_bulk_operation_destroy (bulk);
   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
}


/* test killing a cursor with mongo_cursor_destroy and a real server */
static void
test_kill_cursor_live (void)
{
   mongoc_client_t *client;
   mongoc_collection_t *collection;
   bson_t *b;
   mongoc_bulk_operation_t *bulk;
   int i;
   bson_error_t error;
   uint32_t server_id;
   bool r;
   mongoc_cursor_t *cursor;
   const bson_t *doc;
   int64_t cursor_id;

   client = test_framework_client_new ();
   collection = get_test_collection (client, "test");
   b = tmp_bson ("{}");
   bulk = mongoc_collection_create_bulk_operation (collection, true, NULL);
   for (i = 0; i < 200; i++) {
      mongoc_bulk_operation_insert (bulk, b);
   }

   server_id = mongoc_bulk_operation_execute (bulk, NULL, &error);
   ASSERT_OR_PRINT (server_id > 0, error);

   cursor = mongoc_collection_find (collection,
                                    MONGOC_QUERY_NONE,
                                    0,
                                    0,
                                    0, /* batch size 2 */
                                    b,
                                    NULL,
                                    NULL);

   r = mongoc_cursor_next (cursor, &doc);
   ASSERT (r);
   cursor_id = mongoc_cursor_get_id (cursor);
   ASSERT (cursor_id);

   /* sends OP_KILLCURSORS or killCursors command to server */
   mongoc_cursor_destroy (cursor);

   cursor = _mongoc_cursor_new (client,
                                collection->ns,
                                MONGOC_QUERY_NONE,
                                0,
                                0,
                                0,
                                false,
                                b,
                                NULL,
                                NULL,
                                NULL);

   cursor->rpc.reply.cursor_id = cursor_id;
   cursor->sent = true;
   cursor->end_of_event = true; /* meaning, "finished reading first batch" */
   r = mongoc_cursor_next (cursor, &doc);
   ASSERT (!r);
   ASSERT (mongoc_cursor_error (cursor, &error));
   ASSERT_ERROR_CONTAINS (error, MONGOC_ERROR_CURSOR, 16, "cursor is invalid");

   mongoc_cursor_destroy (cursor);
   mongoc_bulk_operation_destroy (bulk);
   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
}


/* test OP_KILLCURSORS or the killCursors command with mock servers */
static void
_test_kill_cursors (bool pooled, bool use_killcursors_cmd)
{
   mock_rs_t *rs;
   mongoc_client_pool_t *pool = NULL;
   mongoc_client_t *client;
   mongoc_collection_t *collection;
   bson_t *q = BCON_NEW ("a", BCON_INT32 (1));
   mongoc_read_prefs_t *prefs;
   mongoc_cursor_t *cursor;
   const bson_t *doc = NULL;
   future_t *future;
   request_t *request;
   bson_error_t error;
   request_t *kill_cursors;
   const char *ns_out;
   int64_t cursor_id_out;

   rs =
      mock_rs_with_autoismaster (use_killcursors_cmd ? 4 : 3, /* wire version */
                                 true,                        /* has primary */
                                 5,  /* number of secondaries */
                                 0); /* number of arbiters */

   mock_rs_run (rs);

   if (pooled) {
      pool = mongoc_client_pool_new (mock_rs_get_uri (rs));
      client = mongoc_client_pool_pop (pool);
   } else {
      client = mongoc_client_new_from_uri (mock_rs_get_uri (rs));
   }

   collection = mongoc_client_get_collection (client, "db", "collection");

   prefs = mongoc_read_prefs_new (MONGOC_READ_SECONDARY);
   cursor = mongoc_collection_find (
      collection, MONGOC_QUERY_NONE, 0, 0, 0, q, NULL, prefs);

   future = future_cursor_next (cursor, &doc);
   request = mock_rs_receives_request (rs);

   /* reply as appropriate to OP_QUERY or find command */
   mock_rs_replies_to_find (request,
                            MONGOC_QUERY_SLAVE_OK,
                            123,
                            1,
                            "db.collection",
                            "{'b': 1}",
                            use_killcursors_cmd);

   if (!future_get_bool (future)) {
      mongoc_cursor_error (cursor, &error);
      fprintf (stderr, "%s\n", error.message);
      abort ();
   };

   ASSERT_MATCH (doc, "{'b': 1}");
   ASSERT_CMPINT (123, ==, (int) mongoc_cursor_get_id (cursor));

   future_destroy (future);
   future = future_cursor_destroy (cursor);

   if (use_killcursors_cmd) {
      kill_cursors =
         mock_rs_receives_command (rs, "db", MONGOC_QUERY_SLAVE_OK, NULL);

      /* mock server framework can't test "cursors" array, CDRIVER-994 */
      ASSERT (BCON_EXTRACT ((bson_t *) request_get_doc (kill_cursors, 0),
                            "killCursors",
                            BCONE_UTF8 (ns_out),
                            "cursors",
                            "[",
                            BCONE_INT64 (cursor_id_out),
                            "]"));

      ASSERT_CMPSTR ("collection", ns_out);
      ASSERT_CMPINT64 ((int64_t) 123, ==, cursor_id_out);

      mock_rs_replies_simple (request, "{'ok': 1}");
   } else {
      kill_cursors = mock_rs_receives_kill_cursors (rs, 123);
   }

   /* OP_KILLCURSORS was sent to the right secondary */
   ASSERT_CMPINT (request_get_server_port (kill_cursors),
                  ==,
                  request_get_server_port (request));

   BSON_ASSERT (future_wait (future));

   request_destroy (kill_cursors);
   request_destroy (request);
   future_destroy (future);
   mongoc_read_prefs_destroy (prefs);
   mongoc_collection_destroy (collection);
   bson_destroy (q);

   if (pooled) {
      mongoc_client_pool_push (pool, client);
      mongoc_client_pool_destroy (pool);
   } else {
      mongoc_client_destroy (client);
   }

   mock_rs_destroy (rs);
}


static void
test_kill_cursors_single (void)
{
   _test_kill_cursors (false, false);
}


static void
test_kill_cursors_pooled (void)
{
   _test_kill_cursors (true, false);
}


static void
test_kill_cursors_single_cmd (void)
{
   _test_kill_cursors (false, true);
}


static void
test_kill_cursors_pooled_cmd (void)
{
   _test_kill_cursors (true, true);
}


static void
_test_getmore_fail (bool has_primary, bool pooled)
{
   mock_rs_t *rs;
   mongoc_client_pool_t *pool = NULL;
   mongoc_client_t *client;
   mongoc_collection_t *collection;
   bson_t *q = BCON_NEW ("a", BCON_INT32 (1));
   mongoc_read_prefs_t *prefs;
   mongoc_cursor_t *cursor;
   const bson_t *doc = NULL;
   future_t *future;
   request_t *request;

   capture_logs (true);

   /* wire version 0, five secondaries, no arbiters */
   rs = mock_rs_with_autoismaster (0, has_primary, 5, 0);
   mock_rs_run (rs);

   if (pooled) {
      pool = mongoc_client_pool_new (mock_rs_get_uri (rs));
      client = mongoc_client_pool_pop (pool);
   } else {
      client = mongoc_client_new_from_uri (mock_rs_get_uri (rs));
   }

   collection = mongoc_client_get_collection (client, "test", "test");

   prefs = mongoc_read_prefs_new (MONGOC_READ_SECONDARY);
   cursor = mongoc_collection_find (
      collection, MONGOC_QUERY_NONE, 0, 0, 0, q, NULL, prefs);

   future = future_cursor_next (cursor, &doc);
   request = mock_rs_receives_query (
      rs, "test.test", MONGOC_QUERY_SLAVE_OK, 0, 0, "{'a': 1}", NULL);
   BSON_ASSERT (request);

   mock_rs_replies (request, 0, 123, 0, 1, "{'b': 1}");
   BSON_ASSERT (future_get_bool (future));
   ASSERT_MATCH (doc, "{'b': 1}");
   ASSERT_CMPINT (123, ==, (int) mongoc_cursor_get_id (cursor));

   request_destroy (request);
   future_destroy (future);
   future = future_cursor_next (cursor, &doc);
   request = mock_rs_receives_getmore (rs, "test.test", 0, 123);
   BSON_ASSERT (request);
   mock_rs_hangs_up (request);
   BSON_ASSERT (!future_get_bool (future));
   request_destroy (request);

   future_destroy (future);
   future = future_cursor_destroy (cursor);

   /* driver does not reconnect just to send killcursors */
   mock_rs_set_request_timeout_msec (rs, 100);
   BSON_ASSERT (!mock_rs_receives_kill_cursors (rs, 123));

   future_wait (future);
   future_destroy (future);
   mongoc_read_prefs_destroy (prefs);
   mongoc_collection_destroy (collection);
   bson_destroy (q);

   if (pooled) {
      mongoc_client_pool_push (pool, client);
      mongoc_client_pool_destroy (pool);
   } else {
      mongoc_client_destroy (client);
   }

   mock_rs_destroy (rs);
}


static void
test_getmore_fail_with_primary_single (void)
{
   _test_getmore_fail (true, false);
}


static void
test_getmore_fail_with_primary_pooled (void)
{
   _test_getmore_fail (true, true);
}


static void
test_getmore_fail_no_primary_pooled (void)
{
   _test_getmore_fail (false, true);
}


static void
test_getmore_fail_no_primary_single (void)
{
   _test_getmore_fail (false, false);
}


/* We already test that mongoc_cursor_destroy sends OP_KILLCURSORS in
 * test_kill_cursors_single / pooled. Here, test explicit
 * mongoc_client_kill_cursor. */
static void
_test_client_kill_cursor (bool has_primary, bool wire_version_4)
{
   mock_rs_t *rs;
   mongoc_client_t *client;
   mongoc_read_prefs_t *read_prefs;
   bson_error_t error;
   future_t *future;
   request_t *request;

   rs = mock_rs_with_autoismaster (wire_version_4 ? 4 : 3,
                                   has_primary, /* maybe a primary*/
                                   1,           /* definitely a secondary */
                                   0);          /* no arbiter */
   mock_rs_run (rs);
   client = mongoc_client_new_from_uri (mock_rs_get_uri (rs));
   read_prefs = mongoc_read_prefs_new (MONGOC_READ_SECONDARY);

   /* make client open a connection - it won't open one to kill a cursor */
   future = future_client_command_simple (
      client, "admin", tmp_bson ("{'foo': 1}"), read_prefs, NULL, &error);

   request =
      mock_rs_receives_command (rs, "admin", MONGOC_QUERY_SLAVE_OK, NULL);

   mock_rs_replies_simple (request, "{'ok': 1}");
   ASSERT_OR_PRINT (future_get_bool (future), error);
   request_destroy (request);
   future_destroy (future);

   future = future_client_kill_cursor (client, 123);
   mock_rs_set_request_timeout_msec (rs, 100);

   /* we don't pass namespace so client always sends legacy OP_KILLCURSORS */
   request = mock_rs_receives_kill_cursors (rs, 123);

   if (has_primary) {
      BSON_ASSERT (request);

      /* weird but true. see mongoc_client_kill_cursor's documentation */
      BSON_ASSERT (mock_rs_request_is_to_primary (rs, request));

      request_destroy (request); /* server has no reply to OP_KILLCURSORS */
   } else {
      /* TODO: catch and check warning */
      BSON_ASSERT (!request);
   }

   future_wait (future); /* no return value */
   future_destroy (future);
   mongoc_read_prefs_destroy (read_prefs);
   mongoc_client_destroy (client);
   mock_rs_destroy (rs);
}

static void
test_client_kill_cursor_with_primary (void)
{
   _test_client_kill_cursor (true, false);
}


static void
test_client_kill_cursor_without_primary (void)
{
   _test_client_kill_cursor (false, false);
}


static void
test_client_kill_cursor_with_primary_wire_version_4 (void)
{
   _test_client_kill_cursor (true, true);
}


static void
test_client_kill_cursor_without_primary_wire_version_4 (void)
{
   _test_client_kill_cursor (false, true);
}


static int
count_docs (mongoc_cursor_t *cursor)
{
   int n = 0;
   const bson_t *doc;
   bson_error_t error;

   while (mongoc_cursor_next (cursor, &doc)) {
      ++n;
   }

   ASSERT_OR_PRINT (!mongoc_cursor_error (cursor, &error), error);

   return n;
}


static void
_test_cursor_new_from_command (const char *cmd_json,
                               const char *collection_name)
{
   mongoc_client_t *client;
   mongoc_collection_t *collection;
   mongoc_bulk_operation_t *bulk;
   bool r;
   bson_error_t error;
   mongoc_server_description_t *sd;
   uint32_t server_id;
   bson_t reply;
   mongoc_cursor_t *cmd_cursor;

   client = test_framework_client_new ();
   collection = mongoc_client_get_collection (client, "test", collection_name);
   mongoc_collection_remove (
      collection, MONGOC_REMOVE_NONE, tmp_bson ("{}"), NULL, NULL);

   bulk = mongoc_collection_create_bulk_operation (collection, true, NULL);
   mongoc_bulk_operation_insert (bulk, tmp_bson ("{'_id': 'a'}"));
   mongoc_bulk_operation_insert (bulk, tmp_bson ("{'_id': 'b'}"));
   r = (0 != mongoc_bulk_operation_execute (bulk, NULL, &error));
   ASSERT_OR_PRINT (r, error);

   sd = mongoc_topology_select (client->topology, MONGOC_SS_READ, NULL, &error);

   ASSERT_OR_PRINT (sd, error);
   server_id = sd->id;
   mongoc_client_command_simple_with_server_id (
      client, "test", tmp_bson (cmd_json), NULL, server_id, &reply, &error);
   cmd_cursor =
      mongoc_cursor_new_from_command_reply (client, &reply, server_id);
   ASSERT_OR_PRINT (!mongoc_cursor_error (cmd_cursor, &error), error);
   ASSERT_CMPUINT32 (server_id, ==, mongoc_cursor_get_hint (cmd_cursor));
   ASSERT_CMPINT (count_docs (cmd_cursor), ==, 2);

   mongoc_cursor_destroy (cmd_cursor);
   mongoc_server_description_destroy (sd);
   mongoc_bulk_operation_destroy (bulk);
   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
}

static void
test_cursor_empty_collection (void)
{
   mongoc_client_t *client;
   mongoc_collection_t *collection;
   bson_error_t error;
   const bson_t *doc;
   mongoc_cursor_t *cursor;

   client = test_framework_client_new ();
   collection = mongoc_client_get_collection (
      client, "test", "test_cursor_empty_collection");
   mongoc_collection_remove (
      collection, MONGOC_REMOVE_NONE, tmp_bson ("{}"), NULL, NULL);

   cursor = mongoc_collection_find_with_opts (
      collection, tmp_bson ("{}"), NULL, NULL);

   ASSERT (cursor);
   ASSERT (!mongoc_cursor_error (cursor, &error));
   ASSERT (mongoc_cursor_is_alive (cursor));
   ASSERT (mongoc_cursor_more (cursor));

   mongoc_cursor_next (cursor, &doc);

   ASSERT (!mongoc_cursor_error (cursor, &error));
   ASSERT (!mongoc_cursor_is_alive (cursor));
   ASSERT (!mongoc_cursor_more (cursor));

   mongoc_cursor_destroy (cursor);
   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
}


static void
test_cursor_new_from_aggregate (void *ctx)
{
   _test_cursor_new_from_command (
      "{'aggregate': 'test_cursor_new_from_aggregate',"
      " 'pipeline': [], 'cursor': {}}",
      "test_cursor_new_from_aggregate");
}


static void
test_cursor_new_from_aggregate_no_initial (void *ctx)
{
   _test_cursor_new_from_command (
      "{'aggregate': 'test_cursor_new_from_aggregate_no_initial',"
      " 'pipeline': [], 'cursor': {'batchSize': 0}}",
      "test_cursor_new_from_aggregate_no_initial");
}


static void
test_cursor_new_from_find (void *ctx)
{
   _test_cursor_new_from_command ("{'find': 'test_cursor_new_from_find'}",
                                  "test_cursor_new_from_find");
}


static void
test_cursor_new_from_find_batches (void *ctx)
{
   _test_cursor_new_from_command (
      "{'find': 'test_cursor_new_from_find_batches', 'batchSize': 1}",
      "test_cursor_new_from_find_batches");
}


static void
test_cursor_new_invalid (void)
{
   mongoc_client_t *client;
   bson_error_t error;
   mongoc_cursor_t *cursor;
   bson_t b = BSON_INITIALIZER;
   const bson_t *error_doc;

   client = test_framework_client_new ();
   cursor = mongoc_cursor_new_from_command_reply (client, &b, 0);
   ASSERT (cursor);
   ASSERT (mongoc_cursor_error (cursor, &error));
   ASSERT_ERROR_CONTAINS (error,
                          MONGOC_ERROR_CURSOR,
                          MONGOC_ERROR_CURSOR_INVALID_CURSOR,
                          "Couldn't parse cursor document");

   ASSERT (mongoc_cursor_error_document (cursor, &error, &error_doc));
   ASSERT (bson_empty (error_doc));

   mongoc_cursor_destroy (cursor);
   mongoc_client_destroy (client);
}


static void
test_cursor_new_invalid_filter (void)
{
   mongoc_client_t *client;
   mongoc_collection_t *collection;
   mongoc_cursor_t *cursor;
   bson_error_t error;
   const bson_t *error_doc;

   client = test_framework_client_new ();
   collection = mongoc_client_get_collection (client, "test", "test");

   cursor = mongoc_collection_find_with_opts (
      collection, tmp_bson ("{'': 1}"), NULL, NULL);

   ASSERT (cursor);
   ASSERT (mongoc_cursor_error (cursor, &error));
   ASSERT_ERROR_CONTAINS (error,
                          MONGOC_ERROR_CURSOR,
                          MONGOC_ERROR_CURSOR_INVALID_CURSOR,
                          "Invalid filter: empty key");

   ASSERT (mongoc_cursor_error_document (cursor, &error, &error_doc));
   ASSERT (bson_empty (error_doc));

   mongoc_cursor_destroy (cursor);
   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
}


static void
test_cursor_new_invalid_opts (void)
{
   mongoc_client_t *client;
   mongoc_collection_t *collection;
   mongoc_cursor_t *cursor;
   bson_error_t error;
   const bson_t *error_doc;

   client = test_framework_client_new ();
   collection = mongoc_client_get_collection (client, "test", "test");

   cursor = mongoc_collection_find_with_opts (
      collection, tmp_bson (NULL), tmp_bson ("{'projection': {'': 1}}"), NULL);

   ASSERT (cursor);
   ASSERT (mongoc_cursor_error (cursor, &error));
   ASSERT_ERROR_CONTAINS (error,
                          MONGOC_ERROR_CURSOR,
                          MONGOC_ERROR_CURSOR_INVALID_CURSOR,
                          "Invalid opts: empty key");

   ASSERT (mongoc_cursor_error_document (cursor, &error, &error_doc));
   ASSERT (bson_empty (error_doc));
   mongoc_cursor_destroy (cursor);

   cursor = mongoc_collection_find_with_opts (
      collection, tmp_bson (NULL), tmp_bson ("{'$invalid': 1}"), NULL);

   ASSERT (cursor);
   ASSERT (mongoc_cursor_error (cursor, &error));
   ASSERT_ERROR_CONTAINS (error,
                          MONGOC_ERROR_CURSOR,
                          MONGOC_ERROR_CURSOR_INVALID_CURSOR,
                          "Cannot use $-modifiers in opts: \"$invalid\"");

   ASSERT (mongoc_cursor_error_document (cursor, &error, &error_doc));
   ASSERT (bson_empty (error_doc));

   mongoc_cursor_destroy (cursor);

   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
}


static void
test_cursor_new_static (void)
{
   mongoc_client_t *client;
   bson_error_t error;
   mongoc_cursor_t *cursor;
   bson_t *bson_alloced;
   bson_t bson_static;

   bson_alloced = tmp_bson ("{ 'ok':1,"
                            "  'cursor': {"
                            "     'id': 0,"
                            "     'ns': 'test.foo',"
                            "     'firstBatch': [{'x': 1}, {'x': 2}]}}");

   ASSERT (bson_init_static (
      &bson_static, bson_get_data (bson_alloced), bson_alloced->len));

   /* test heap-allocated bson */
   client = test_framework_client_new ();
   cursor = mongoc_cursor_new_from_command_reply (
      client, bson_copy (bson_alloced), 0);

   ASSERT (cursor);
   ASSERT (!mongoc_cursor_error (cursor, &error));
   mongoc_cursor_destroy (cursor);

   /* test static bson */
   cursor = mongoc_cursor_new_from_command_reply (client, &bson_static, 0);
   ASSERT (cursor);
   ASSERT (!mongoc_cursor_error (cursor, &error));

   mongoc_cursor_destroy (cursor);
   mongoc_client_destroy (client);
}


static void
test_cursor_hint_errors (void)
{
   mongoc_client_t *client;
   mongoc_collection_t *collection;
   mongoc_cursor_t *cursor;

   client = test_framework_client_new ();
   collection = mongoc_client_get_collection (client, "db", "collection");
   cursor = mongoc_collection_find (
      collection, MONGOC_QUERY_NONE, 0, 0, 0, tmp_bson ("{}"), NULL, NULL);

   capture_logs (true);
   ASSERT (!mongoc_cursor_set_hint (cursor, 0));
   ASSERT_CAPTURED_LOG ("mongoc_cursor_set_hint",
                        MONGOC_LOG_LEVEL_ERROR,
                        "cannot set server_id to 0");

   capture_logs (true); /* clear logs */
   ASSERT (mongoc_cursor_set_hint (cursor, 123));
   ASSERT_CMPUINT32 ((uint32_t) 123, ==, mongoc_cursor_get_hint (cursor));
   ASSERT_NO_CAPTURED_LOGS ("mongoc_cursor_set_hint");
   ASSERT (!mongoc_cursor_set_hint (cursor, 42));
   ASSERT_CAPTURED_LOG ("mongoc_cursor_set_hint",
                        MONGOC_LOG_LEVEL_ERROR,
                        "server_id already set");

   /* last set_hint had no effect */
   ASSERT_CMPUINT32 ((uint32_t) 123, ==, mongoc_cursor_get_hint (cursor));

   mongoc_cursor_destroy (cursor);
   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
}


static uint32_t
server_id_for_read_mode (mongoc_client_t *client, mongoc_read_mode_t read_mode)
{
   mongoc_read_prefs_t *prefs;
   mongoc_server_description_t *sd;
   bson_error_t error;
   uint32_t server_id;

   prefs = mongoc_read_prefs_new (read_mode);
   sd =
      mongoc_topology_select (client->topology, MONGOC_SS_READ, prefs, &error);

   ASSERT_OR_PRINT (sd, error);
   server_id = sd->id;

   mongoc_server_description_destroy (sd);
   mongoc_read_prefs_destroy (prefs);

   return server_id;
}


static void
_test_cursor_hint (bool pooled, bool use_primary)
{
   mock_rs_t *rs;
   mongoc_client_pool_t *pool = NULL;
   mongoc_client_t *client;
   mongoc_collection_t *collection;
   bson_t *q = BCON_NEW ("a", BCON_INT32 (1));
   mongoc_cursor_t *cursor;
   uint32_t server_id;
   mongoc_query_flags_t expected_flags;
   const bson_t *doc = NULL;
   future_t *future;
   request_t *request;

   /* wire version 0, primary, two secondaries, no arbiters */
   rs = mock_rs_with_autoismaster (0, true, 2, 0);
   mock_rs_run (rs);

   if (pooled) {
      pool = mongoc_client_pool_new (mock_rs_get_uri (rs));
      client = mongoc_client_pool_pop (pool);
   } else {
      client = mongoc_client_new_from_uri (mock_rs_get_uri (rs));
   }

   collection = mongoc_client_get_collection (client, "test", "test");

   cursor = mongoc_collection_find (
      collection, MONGOC_QUERY_NONE, 0, 0, 0, q, NULL, NULL);
   ASSERT_CMPUINT32 ((uint32_t) 0, ==, mongoc_cursor_get_hint (cursor));

   if (use_primary) {
      server_id = server_id_for_read_mode (client, MONGOC_READ_PRIMARY);
      expected_flags = MONGOC_QUERY_NONE;
   } else {
      server_id = server_id_for_read_mode (client, MONGOC_READ_SECONDARY);
      expected_flags = MONGOC_QUERY_SLAVE_OK;
   }

   ASSERT (mongoc_cursor_set_hint (cursor, server_id));
   ASSERT_CMPUINT32 (server_id, ==, mongoc_cursor_get_hint (cursor));

   future = future_cursor_next (cursor, &doc);
   request = mock_rs_receives_query (
      rs, "test.test", expected_flags, 0, 0, "{'a': 1}", NULL);

   if (use_primary) {
      BSON_ASSERT (mock_rs_request_is_to_primary (rs, request));
   } else {
      BSON_ASSERT (mock_rs_request_is_to_secondary (rs, request));
   }

   mock_rs_replies (request, 0, 0, 0, 1, "{'b': 1}");
   BSON_ASSERT (future_get_bool (future));
   ASSERT_MATCH (doc, "{'b': 1}");

   request_destroy (request);
   future_destroy (future);
   mongoc_cursor_destroy (cursor);
   mongoc_collection_destroy (collection);

   if (pooled) {
      mongoc_client_pool_push (pool, client);
      mongoc_client_pool_destroy (pool);
   } else {
      mongoc_client_destroy (client);
   }

   mock_rs_destroy (rs);
   bson_destroy (q);
}

static void
test_hint_single_secondary (void)
{
   _test_cursor_hint (false, false);
}

static void
test_hint_single_primary (void)
{
   _test_cursor_hint (false, true);
}

static void
test_hint_pooled_secondary (void)
{
   _test_cursor_hint (true, false);
}

static void
test_hint_pooled_primary (void)
{
   _test_cursor_hint (true, true);
}

mongoc_read_mode_t modes[] = {
   MONGOC_READ_PRIMARY,
   MONGOC_READ_PRIMARY_PREFERRED,
   MONGOC_READ_SECONDARY,
   MONGOC_READ_SECONDARY_PREFERRED,
   MONGOC_READ_NEAREST,
};

mongoc_query_flags_t expected_flag[] = {
   MONGOC_QUERY_NONE,
   MONGOC_QUERY_SLAVE_OK,
   MONGOC_QUERY_SLAVE_OK,
   MONGOC_QUERY_SLAVE_OK,
   MONGOC_QUERY_SLAVE_OK,
};

/* test that mongoc_cursor_set_hint sets slaveok for mongos only if read pref
 * is secondaryPreferred. */
static void
test_cursor_hint_mongos (void)
{
   mock_server_t *server;
   mongoc_client_t *client;
   mongoc_collection_t *collection;
   size_t i;
   mongoc_read_prefs_t *prefs;
   mongoc_cursor_t *cursor;
   const bson_t *doc = NULL;
   future_t *future;
   request_t *request;

   server = mock_mongos_new (0);
   mock_server_run (server);
   client = mongoc_client_new_from_uri (mock_server_get_uri (server));
   collection = mongoc_client_get_collection (client, "test", "test");

   for (i = 0; i < sizeof (modes) / sizeof (mongoc_read_mode_t); i++) {
      prefs = mongoc_read_prefs_new (modes[i]);
      cursor = mongoc_collection_find (
         collection, MONGOC_QUERY_NONE, 0, 0, 0, tmp_bson (NULL), NULL, prefs);

      ASSERT_CMPUINT32 ((uint32_t) 0, ==, mongoc_cursor_get_hint (cursor));
      ASSERT (mongoc_cursor_set_hint (cursor, 1));
      ASSERT_CMPUINT32 ((uint32_t) 1, ==, mongoc_cursor_get_hint (cursor));

      future = future_cursor_next (cursor, &doc);

      request = mock_server_receives_query (
         server, "test.test", expected_flag[i], 0, 0, "{}", NULL);

      mock_server_replies_simple (request, "{}");
      BSON_ASSERT (future_get_bool (future));

      request_destroy (request);
      future_destroy (future);
      mongoc_cursor_destroy (cursor);
      mongoc_read_prefs_destroy (prefs);
   }

   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
   mock_server_destroy (server);
}

static void
test_cursor_hint_mongos_cmd (void)
{
   mock_server_t *server;
   mongoc_client_t *client;
   mongoc_collection_t *collection;
   mongoc_cursor_t *cursor;
   size_t i;
   mongoc_read_prefs_t *prefs;
   const bson_t *doc = NULL;
   future_t *future;
   request_t *request;

   server = mock_mongos_new (WIRE_VERSION_FIND_CMD);
   mock_server_run (server);
   client = mongoc_client_new_from_uri (mock_server_get_uri (server));
   collection = mongoc_client_get_collection (client, "test", "test");

   for (i = 0; i < sizeof (modes) / sizeof (mongoc_read_mode_t); i++) {
      prefs = mongoc_read_prefs_new (modes[i]);
      cursor = mongoc_collection_find (
         collection, MONGOC_QUERY_NONE, 0, 0, 0, tmp_bson (NULL), NULL, prefs);

      ASSERT_CMPUINT32 ((uint32_t) 0, ==, mongoc_cursor_get_hint (cursor));
      ASSERT (mongoc_cursor_set_hint (cursor, 1));
      ASSERT_CMPUINT32 ((uint32_t) 1, ==, mongoc_cursor_get_hint (cursor));

      future = future_cursor_next (cursor, &doc);

      request = mock_server_receives_command (
         server, "test", expected_flag[i], 0, 0, "{'find': 'test'}", NULL);

      mock_server_replies_simple (request,
                                  "{'ok': 1,"
                                  " 'cursor': {"
                                  "    'id': 0,"
                                  "    'ns': 'test.test',"
                                  "    'firstBatch': [{}]}}");

      BSON_ASSERT (future_get_bool (future));

      request_destroy (request);
      future_destroy (future);
      mongoc_cursor_destroy (cursor);
      mongoc_read_prefs_destroy (prefs);
   }

   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
   mock_server_destroy (server);
}
/* Tests CDRIVER-562: after calling ismaster to handshake a new connection we
 * must update topology description with the server response. If not, this test
 * fails under auth with "auth failed" because we use the wrong auth protocol.
 */
static void
_test_cursor_hint_no_warmup (bool pooled)
{
   mongoc_client_pool_t *pool = NULL;
   mongoc_client_t *client;
   mongoc_collection_t *collection;
   bson_t *q = tmp_bson (NULL);
   mongoc_cursor_t *cursor;
   const bson_t *doc = NULL;
   bson_error_t error;

   if (pooled) {
      pool = test_framework_client_pool_new ();
      client = mongoc_client_pool_pop (pool);
   } else {
      client = test_framework_client_new ();
   }

   collection = get_test_collection (client, "test_cursor_hint_no_warmup");
   cursor = mongoc_collection_find (
      collection, MONGOC_QUERY_NONE, 0, 0, 0, q, NULL, NULL);

   /* no chance for topology scan, no server selection */
   ASSERT (mongoc_cursor_set_hint (cursor, 1));
   ASSERT_CMPUINT32 (1, ==, mongoc_cursor_get_hint (cursor));

   mongoc_cursor_next (cursor, &doc);
   ASSERT_OR_PRINT (!mongoc_cursor_error (cursor, &error), error);

   mongoc_cursor_destroy (cursor);
   mongoc_collection_destroy (collection);

   if (pooled) {
      mongoc_client_pool_push (pool, client);
      mongoc_client_pool_destroy (pool);
   } else {
      mongoc_client_destroy (client);
   }
}

static void
test_hint_no_warmup_single (void)
{
   _test_cursor_hint_no_warmup (false);
}

static void
test_hint_no_warmup_pooled (void)
{
   _test_cursor_hint_no_warmup (true);
}

static void
test_tailable_alive (void)
{
   mongoc_client_t *client;
   mongoc_database_t *database;
   char *collection_name;
   mongoc_collection_t *collection;
   bool r;
   bson_error_t error;
   mongoc_cursor_t *cursor;
   const bson_t *doc;

   client = test_framework_client_new ();
   database = mongoc_client_get_database (client, "test");
   collection_name = gen_collection_name ("test");

   collection = mongoc_database_get_collection (database, collection_name);
   mongoc_collection_drop (collection, NULL);
   mongoc_collection_destroy (collection);

   collection = mongoc_database_create_collection (
      database,
      collection_name,
      tmp_bson ("{'capped': true, 'size': 10000}"),
      &error);

   ASSERT_OR_PRINT (collection, error);

   r = mongoc_collection_insert (
      collection, MONGOC_INSERT_NONE, tmp_bson ("{}"), NULL, &error);

   ASSERT_OR_PRINT (r, error);

   /* test mongoc_collection_find and mongoc_collection_find_with_opts */
   cursor = mongoc_collection_find (collection,
                                    MONGOC_QUERY_TAILABLE_CURSOR |
                                       MONGOC_QUERY_AWAIT_DATA,
                                    0,
                                    0,
                                    0,
                                    tmp_bson (NULL),
                                    NULL,
                                    NULL);

   ASSERT (mongoc_cursor_is_alive (cursor));
   ASSERT (mongoc_cursor_next (cursor, &doc));

   /* still alive */
   ASSERT (mongoc_cursor_is_alive (cursor));
   ASSERT (mongoc_cursor_more (cursor));

   /* no next document, but still alive and could return more in the future
    * see CDRIVER-1530 */
   ASSERT (!mongoc_cursor_next (cursor, &doc));
   ASSERT (mongoc_cursor_is_alive (cursor));
   ASSERT (mongoc_cursor_more (cursor));

   mongoc_cursor_destroy (cursor);

   cursor = mongoc_collection_find_with_opts (
      collection,
      tmp_bson (NULL),
      tmp_bson ("{'tailable': true, 'awaitData': true}"),
      NULL);

   ASSERT (mongoc_cursor_is_alive (cursor));
   ASSERT (mongoc_cursor_next (cursor, &doc));

   /* still alive */
   ASSERT (mongoc_cursor_is_alive (cursor));

   mongoc_cursor_destroy (cursor);

   mongoc_collection_destroy (collection);
   mongoc_database_destroy (database);
   bson_free (collection_name);
   mongoc_client_destroy (client);
}


typedef struct {
   int64_t skip;
   int64_t limit;
   int64_t batch_size;
   int64_t expected_n_return[3];
   int64_t reply_length[3];
} cursor_n_return_test;


static bson_t *
_make_n_empty_docs (int64_t n)
{
   int64_t i;
   bson_t *docs;

   docs = bson_malloc (n * sizeof (bson_t));
   for (i = 0; i < n; i++) {
      bson_init (&docs[i]);
   }

   return docs;
}


static void
_test_cursor_n_return_op_query (mongoc_cursor_t *cursor,
                                mock_server_t *server,
                                cursor_n_return_test *test)
{
   bson_t *reply_docs;
   const bson_t *doc;
   request_t *request;
   future_t *future;
   int i;
   int reply_no;
   bool cursor_finished;


   future = future_cursor_next (cursor, &doc);
   request = mock_server_receives_query (server,
                                         "db.coll",
                                         MONGOC_QUERY_SLAVE_OK,
                                         (uint32_t) test->skip,
                                         (uint32_t) test->expected_n_return[0],
                                         NULL,
                                         NULL);

   reply_docs = _make_n_empty_docs (test->reply_length[0]);
   mock_server_reply_multi (request,
                            MONGOC_REPLY_NONE,
                            reply_docs,
                            (uint32_t) test->reply_length[0],
                            123 /* cursor_id */);

   ASSERT (future_get_bool (future));
   bson_free (reply_docs);
   future_destroy (future);
   request_destroy (request);

   /* advance to the end of the batch */
   for (i = 1; i < test->reply_length[0]; i++) {
      ASSERT (mongoc_cursor_next (cursor, &doc));
   }

   for (reply_no = 1; reply_no < 3; reply_no++) {
      /* expect op_getmore, send reply_length[reply_no] docs to client */
      future = future_cursor_next (cursor, &doc);
      request = mock_server_receives_getmore (
         server, "db.coll", (uint32_t) test->expected_n_return[reply_no], 123);

      reply_docs = _make_n_empty_docs (test->reply_length[reply_no]);
      cursor_finished = (reply_no == 2);
      mock_server_reply_multi (request,
                               MONGOC_REPLY_NONE,
                               reply_docs,
                               (uint32_t) test->reply_length[reply_no],
                               cursor_finished ? 0 : 123);

      ASSERT (future_get_bool (future));
      bson_free (reply_docs);
      future_destroy (future);
      request_destroy (request);

      /* advance to the end of the batch */
      for (i = 1; i < test->reply_length[reply_no]; i++) {
         ASSERT (mongoc_cursor_next (cursor, &doc));
      }
   }
}


static void
_make_reply_batch (bson_string_t *reply,
                   uint32_t n_docs,
                   bool first_batch,
                   bool finished)
{
   uint32_t j;

   bson_string_append_printf (reply,
                              "{'ok': 1, 'cursor': {"
                              "    'id': %d,"
                              "    'ns': 'db.coll',",
                              finished ? 0 : 123);

   if (first_batch) {
      bson_string_append (reply, "'firstBatch': [{}");
   } else {
      bson_string_append (reply, "'nextBatch': [{}");
   }

   for (j = 1; j < n_docs; j++) {
      bson_string_append (reply, ", {}");
   }

   bson_string_append (reply, "]}}");
}


static void
_test_cursor_n_return_find_cmd (mongoc_cursor_t *cursor,
                                mock_server_t *server,
                                cursor_n_return_test *test)
{
   bson_t find_cmd = BSON_INITIALIZER;
   bson_t getmore_cmd = BSON_INITIALIZER;
   const bson_t *doc;
   request_t *request;
   future_t *future;
   int j;
   int reply_no;
   bson_string_t *reply;
   bool cursor_finished;

   BSON_APPEND_UTF8 (&find_cmd, "find", "coll");
   if (test->skip) {
      BSON_APPEND_INT64 (&find_cmd, "skip", test->skip);
   }
   if (test->limit > 0) {
      BSON_APPEND_INT64 (&find_cmd, "limit", test->limit);
   } else if (test->limit < 0) {
      BSON_APPEND_INT64 (&find_cmd, "limit", -test->limit);
      BSON_APPEND_BOOL (&find_cmd, "singleBatch", true);
   }

   if (test->batch_size) {
      BSON_APPEND_INT64 (&find_cmd, "batchSize", BSON_ABS (test->batch_size));
   }

   future = future_cursor_next (cursor, &doc);
   request =
      mock_server_receives_command (server, "db", MONGOC_QUERY_SLAVE_OK, NULL);

   ASSERT (match_bson (request_get_doc (request, 0), &find_cmd, true));

   reply = bson_string_new (NULL);
   _make_reply_batch (reply, (uint32_t) test->reply_length[0], true, false);
   mock_server_replies_simple (request, reply->str);
   bson_string_free (reply, true);

   ASSERT (future_get_bool (future));
   future_destroy (future);
   request_destroy (request);

   /* advance to the end of the batch */
   for (j = 1; j < test->reply_length[0]; j++) {
      ASSERT (mongoc_cursor_next (cursor, &doc));
   }

   for (reply_no = 1; reply_no < 3; reply_no++) {
      /* expect getMore command, send reply_length[reply_no] docs to client */
      future = future_cursor_next (cursor, &doc);
      request = mock_server_receives_command (
         server, "db", MONGOC_QUERY_SLAVE_OK, NULL);

      bson_reinit (&getmore_cmd);
      BSON_APPEND_INT64 (&getmore_cmd, "getMore", 123);
      if (test->expected_n_return[reply_no] && test->batch_size) {
         BSON_APPEND_INT64 (&getmore_cmd,
                            "batchSize",
                            BSON_ABS (test->expected_n_return[reply_no]));
      } else {
         BSON_APPEND_DOCUMENT (
            &getmore_cmd, "batchSize", tmp_bson ("{'$exists': false}"));
      }

      ASSERT (match_bson (request_get_doc (request, 0), &getmore_cmd, true));

      reply = bson_string_new (NULL);
      cursor_finished = (reply_no == 2);
      _make_reply_batch (reply,
                         (uint32_t) test->reply_length[reply_no],
                         false,
                         cursor_finished);

      mock_server_replies_simple (request, reply->str);
      bson_string_free (reply, true);

      ASSERT (future_get_bool (future));
      future_destroy (future);
      request_destroy (request);

      /* advance to the end of the batch */
      for (j = 1; j < test->reply_length[reply_no]; j++) {
         ASSERT (mongoc_cursor_next (cursor, &doc));
      }
   }

   bson_destroy (&find_cmd);
   bson_destroy (&getmore_cmd);
}


static void
_test_cursor_n_return (bool find_cmd, bool find_with_opts)
{
   cursor_n_return_test tests[] = {{
                                      0,         /* skip              */
                                      0,         /* limit             */
                                      0,         /* batch_size        */
                                      {0, 0, 0}, /* expected_n_return */
                                      {1, 1, 1}  /* reply_length      */
                                   },
                                   {
                                      7,         /* skip              */
                                      0,         /* limit             */
                                      0,         /* batch_size        */
                                      {0, 0, 0}, /* expected_n_return */
                                      {1, 1, 1}  /* reply_length      */
                                   },
                                   {
                                      0,         /* skip              */
                                      3,         /* limit             */
                                      0,         /* batch_size        */
                                      {3, 2, 1}, /* expected_n_return */
                                      {1, 1, 1}  /* reply_length      */
                                   },
                                   {
                                      0,         /* skip              */
                                      5,         /* limit             */
                                      2,         /* batch_size        */
                                      {2, 2, 1}, /* expected_n_return */
                                      {2, 2, 1}  /* reply_length      */
                                   },
                                   {
                                      0,         /* skip              */
                                      4,         /* limit             */
                                      7,         /* batch_size        */
                                      {4, 2, 1}, /* expected_n_return */
                                      {2, 1, 1}  /* reply_length      */
                                   },
                                   {
                                      0,            /* skip              */
                                      -3,           /* limit             */
                                      1,            /* batch_size        */
                                      {-3, -3, -3}, /* expected_n_return */
                                      {1, 1, 1}     /* reply_length      */
                                   }};

   cursor_n_return_test *test;
   size_t i;
   mock_server_t *server;
   mongoc_client_t *client;
   mongoc_collection_t *collection;
   bson_t opts = BSON_INITIALIZER;
   mongoc_cursor_t *cursor;

   server =
      mock_server_with_autoismaster (find_cmd ? WIRE_VERSION_FIND_CMD : 0);

   mock_server_run (server);

   client = mongoc_client_new_from_uri (mock_server_get_uri (server));
   collection = mongoc_client_get_collection (client, "db", "coll");

   for (i = 0; i < sizeof (tests) / sizeof (cursor_n_return_test); i++) {
      test = &tests[i];

      if (find_with_opts) {
         bson_reinit (&opts);

         if (test->skip) {
            BSON_APPEND_INT64 (&opts, "skip", test->skip);
         }

         if (test->limit > 0) {
            BSON_APPEND_INT64 (&opts, "limit", test->limit);
         } else if (test->limit < 0) {
            BSON_APPEND_INT64 (&opts, "limit", -test->limit);
            BSON_APPEND_BOOL (&opts, "singleBatch", true);
         }

         if (test->batch_size) {
            BSON_APPEND_INT64 (&opts, "batchSize", test->batch_size);
         }

         cursor = mongoc_collection_find_with_opts (
            collection, tmp_bson (NULL), &opts, NULL);
      } else {
         cursor = mongoc_collection_find (collection,
                                          MONGOC_QUERY_NONE,
                                          (uint32_t) test->skip,
                                          (uint32_t) test->limit,
                                          (uint32_t) test->batch_size,
                                          tmp_bson (NULL),
                                          NULL,
                                          NULL);
      }

      if (find_cmd) {
         _test_cursor_n_return_find_cmd (cursor, server, test);
      } else {
         _test_cursor_n_return_op_query (cursor, server, test);
      }

      mongoc_cursor_destroy (cursor);
   }

   bson_destroy (&opts);
   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
   mock_server_destroy (server);
}


static void
test_n_return_op_query (void)
{
   _test_cursor_n_return (false, false);
}


static void
test_n_return_op_query_with_opts (void)
{
   _test_cursor_n_return (false, true);
}


static void
test_n_return_find_cmd (void)
{
   _test_cursor_n_return (true, false);
}


static void
test_n_return_find_cmd_with_opts (void)
{
   _test_cursor_n_return (true, true);
}


static void
test_error_document_query (void)
{
   mongoc_client_t *client;
   mongoc_collection_t *collection;
   mongoc_cursor_t *cursor;
   bson_error_t error;
   const bson_t *doc;
   const bson_t *error_doc;

   client = test_framework_client_new ();
   mongoc_client_set_error_api (client, 2);
   collection = get_test_collection (client, "test_error_document_query");
   cursor = mongoc_collection_find_with_opts (
      collection, tmp_bson ("{'x': {'$badOperator': 1}}"), NULL, NULL);

   ASSERT (!mongoc_cursor_next (cursor, &doc));
   ASSERT (mongoc_cursor_error_document (cursor, &error, &error_doc));
   ASSERT_CMPUINT32 (error.domain, ==, MONGOC_ERROR_SERVER);
   ASSERT_CONTAINS (error.message, "$badOperator");
   ASSERT_CMPINT32 (
      bson_lookup_int32 (error_doc, "code"), ==, (int32_t) error.code);

   mongoc_cursor_destroy (cursor);
   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
}


static void
test_error_document_command (void)
{
   mongoc_client_t *client;
   mongoc_cursor_t *cursor;
   bson_error_t error;
   const bson_t *doc;
   const bson_t *error_doc;

   client = test_framework_client_new ();
   mongoc_client_set_error_api (client, 2);
   cursor = mongoc_client_command (client,
                                   "test",
                                   MONGOC_QUERY_NONE,
                                   0,
                                   0,
                                   0,
                                   tmp_bson ("{'foo': 1}"), /* no such cmd */
                                   NULL,
                                   NULL);

   ASSERT (!mongoc_cursor_next (cursor, &doc));
   ASSERT (mongoc_cursor_error_document (cursor, &error, &error_doc));
   ASSERT_CMPUINT32 (error.domain, ==, MONGOC_ERROR_SERVER);
   ASSERT_CONTAINS (error.message, "no such");

   /* MongoDB 2.4 has no "code" in the reply for "no such command" */
   if (test_framework_max_wire_version_at_least (2)) {
      ASSERT_CMPINT32 (bson_lookup_int32 (error_doc, "code"), ==,
                       (int32_t) error.code);
   }

   mongoc_cursor_destroy (cursor);
   mongoc_client_destroy (client);
}


static void
test_error_document_getmore (void)
{
   mongoc_client_t *client;
   mongoc_collection_t *collection;
   int i;
   bool r;
   bson_error_t error;
   mongoc_cursor_t *cursor;
   const bson_t *doc;
   const bson_t *error_doc;

   client = test_framework_client_new ();
   mongoc_client_set_error_api (client, 2);
   collection = get_test_collection (client, "test_error_document_getmore");
   mongoc_collection_drop (collection, NULL);

   for (i = 0; i < 10; i++) {
      r = mongoc_collection_insert (collection,
                                    MONGOC_INSERT_NONE,
                                    tmp_bson ("{'i': %d}", i),
                                    NULL,
                                    &error);

      ASSERT_OR_PRINT (r, error);
   }

   cursor = mongoc_collection_find_with_opts (
      collection, tmp_bson ("{}"), tmp_bson ("{'batchSize': 2}"), NULL);

   ASSERT (mongoc_cursor_next (cursor, &doc));

   mongoc_collection_drop (collection, NULL);

   ASSERT (mongoc_cursor_next (cursor, &doc));
   ASSERT (!mongoc_cursor_next (cursor, &doc));
   ASSERT (mongoc_cursor_error_document (cursor, &error, &error_doc));

   /* results vary by server version */
   if (error.domain == MONGOC_ERROR_CURSOR) {
      /* MongoDB 3.0 and older */
      ASSERT_ERROR_CONTAINS (error,
                             MONGOC_ERROR_CURSOR,
                             MONGOC_ERROR_CURSOR_INVALID_CURSOR,
                             "cursor is invalid");
   } else {
      /* MongoDB 3.2+ */
      ASSERT_CMPUINT32 (error.domain, ==, MONGOC_ERROR_SERVER);
      ASSERT_CMPINT32 (
         bson_lookup_int32 (error_doc, "code"), ==, (int32_t) error.code);
   }

   mongoc_cursor_destroy (cursor);
   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
}


void
test_cursor_install (TestSuite *suite)
{
   TestSuite_AddLive (suite, "/Cursor/get_host", test_get_host);
   TestSuite_AddLive (suite, "/Cursor/clone", test_clone);
   TestSuite_AddLive (suite, "/Cursor/limit", test_limit);
   TestSuite_AddLive (suite, "/Cursor/kill/live", test_kill_cursor_live);
   TestSuite_AddMockServerTest (
      suite, "/Cursor/kill/single", test_kill_cursors_single);
   TestSuite_AddMockServerTest (
      suite, "/Cursor/kill/pooled", test_kill_cursors_pooled);
   TestSuite_AddMockServerTest (
      suite, "/Cursor/kill/single/cmd", test_kill_cursors_single_cmd);
   TestSuite_AddMockServerTest (
      suite, "/Cursor/kill/pooled/cmd", test_kill_cursors_pooled_cmd);
   TestSuite_AddMockServerTest (suite,
                                "/Cursor/getmore_fail/with_primary/pooled",
                                test_getmore_fail_with_primary_pooled);
   TestSuite_AddMockServerTest (suite,
                                "/Cursor/getmore_fail/with_primary/single",
                                test_getmore_fail_with_primary_single);
   TestSuite_AddMockServerTest (suite,
                                "/Cursor/getmore_fail/no_primary/pooled",
                                test_getmore_fail_no_primary_pooled);
   TestSuite_AddMockServerTest (suite,
                                "/Cursor/getmore_fail/no_primary/single",
                                test_getmore_fail_no_primary_single);
   TestSuite_AddMockServerTest (suite,
                                "/Cursor/client_kill_cursor/with_primary",
                                test_client_kill_cursor_with_primary);
   TestSuite_AddMockServerTest (suite,
                                "/Cursor/client_kill_cursor/without_primary",
                                test_client_kill_cursor_without_primary);
   TestSuite_AddMockServerTest (
      suite,
      "/Cursor/client_kill_cursor/with_primary/wv4",
      test_client_kill_cursor_with_primary_wire_version_4);
   TestSuite_AddMockServerTest (
      suite,
      "/Cursor/client_kill_cursor/without_primary/wv4",
      test_client_kill_cursor_without_primary_wire_version_4);
   TestSuite_AddLive (
      suite, "/Cursor/empty_collection", test_cursor_empty_collection);
   TestSuite_AddFull (suite,
                      "/Cursor/new_from_agg",
                      test_cursor_new_from_aggregate,
                      NULL,
                      NULL,
                      test_framework_skip_if_max_wire_version_less_than_2);
   TestSuite_AddFull (suite,
                      "/Cursor/new_from_agg_no_initial",
                      test_cursor_new_from_aggregate_no_initial,
                      NULL,
                      NULL,
                      test_framework_skip_if_max_wire_version_less_than_2);
   TestSuite_AddFull (suite,
                      "/Cursor/new_from_find",
                      test_cursor_new_from_find,
                      NULL,
                      NULL,
                      test_framework_skip_if_max_wire_version_less_than_4);
   TestSuite_AddFull (suite,
                      "/Cursor/new_from_find_batches",
                      test_cursor_new_from_find_batches,
                      NULL,
                      NULL,
                      test_framework_skip_if_max_wire_version_less_than_4);
   TestSuite_AddLive (suite, "/Cursor/new_invalid", test_cursor_new_invalid);
   TestSuite_AddLive (
      suite, "/Cursor/new_invalid_filter", test_cursor_new_invalid_filter);
   TestSuite_AddLive (
      suite, "/Cursor/new_invalid_opts", test_cursor_new_invalid_opts);
   TestSuite_AddLive (suite, "/Cursor/new_static", test_cursor_new_static);
   TestSuite_AddLive (suite, "/Cursor/hint/errors", test_cursor_hint_errors);
   TestSuite_AddMockServerTest (
      suite, "/Cursor/hint/single/secondary", test_hint_single_secondary);
   TestSuite_AddMockServerTest (
      suite, "/Cursor/hint/single/primary", test_hint_single_primary);
   TestSuite_AddMockServerTest (
      suite, "/Cursor/hint/pooled/secondary", test_hint_pooled_secondary);
   TestSuite_AddMockServerTest (
      suite, "/Cursor/hint/pooled/primary", test_hint_pooled_primary);
   TestSuite_AddMockServerTest (
      suite, "/Cursor/hint/mongos", test_cursor_hint_mongos);
   TestSuite_AddMockServerTest (
      suite, "/Cursor/hint/mongos/cmd", test_cursor_hint_mongos_cmd);
   TestSuite_AddLive (
      suite, "/Cursor/hint/no_warmup/single", test_hint_no_warmup_single);
   TestSuite_AddLive (
      suite, "/Cursor/hint/no_warmup/pooled", test_hint_no_warmup_pooled);
   TestSuite_AddLive (suite, "/Cursor/tailable/alive", test_tailable_alive);
   TestSuite_AddMockServerTest (
      suite, "/Cursor/n_return/op_query", test_n_return_op_query);
   TestSuite_AddMockServerTest (suite,
                                "/Cursor/n_return/op_query/with_opts",
                                test_n_return_op_query_with_opts);
   TestSuite_AddMockServerTest (
      suite, "/Cursor/n_return/find_cmd", test_n_return_find_cmd);
   TestSuite_AddMockServerTest (suite,
                                "/Cursor/n_return/find_cmd/with_opts",
                                test_n_return_find_cmd_with_opts);
   TestSuite_AddLive (
      suite, "/Cursor/error_document/query", test_error_document_query);
   TestSuite_AddLive (
      suite, "/Cursor/error_document/getmore", test_error_document_getmore);
   TestSuite_AddLive (
      suite, "/Cursor/error_document/command", test_error_document_command);
}
