#include <fcntl.h>
#include <mongoc.h>

#include "mongoc-client-private.h"
#include "mongoc-cursor-private.h"
#include "mongoc-uri-private.h"
#include "mongoc-util-private.h"

#include "TestSuite.h"
#include "test-conveniences.h"
#include "test-libmongoc.h"
#include "mock_server/future.h"
#include "mock_server/future-functions.h"
#include "mock_server/mock-server.h"


#undef MONGOC_LOG_DOMAIN
#define MONGOC_LOG_DOMAIN "exhaust-test"


int
skip_if_mongos (void)
{
   if (!TestSuite_CheckLive ()) {
      return 0;
   }
   return test_framework_is_mongos () ? 0 : 1;
}


static int64_t
get_timestamp (mongoc_client_t *client, mongoc_cursor_t *cursor)
{
   uint32_t server_id;

   server_id = mongoc_cursor_get_hint (cursor);

   if (client->topology->single_threaded) {
      mongoc_topology_scanner_node_t *scanner_node;

      scanner_node = mongoc_topology_scanner_get_node (
         client->topology->scanner, server_id);

      return scanner_node->timestamp;
   } else {
      mongoc_cluster_node_t *cluster_node;

      cluster_node = (mongoc_cluster_node_t *) mongoc_set_get (
         client->cluster.nodes, server_id);

      return cluster_node->timestamp;
   }
}


static void
test_exhaust_cursor (bool pooled)
{
   mongoc_stream_t *stream;
   mongoc_write_concern_t *wr;
   mongoc_client_t *client;
   mongoc_client_pool_t *pool = NULL;
   mongoc_collection_t *collection;
   mongoc_cursor_t *cursor;
   mongoc_cursor_t *cursor2;
   const bson_t *doc;
   bson_t q;
   bson_t b[10];
   bson_t *bptr[10];
   int i;
   bool r;
   uint32_t server_id;
   bson_error_t error;
   bson_oid_t oid;
   int64_t timestamp1;

   if (pooled) {
      pool = test_framework_client_pool_new ();
      client = mongoc_client_pool_pop (pool);
   } else {
      client = test_framework_client_new ();
   }
   BSON_ASSERT (client);

   collection = get_test_collection (client, "test_exhaust_cursor");
   BSON_ASSERT (collection);

   mongoc_collection_drop (collection, &error);

   wr = mongoc_write_concern_new ();
   mongoc_write_concern_set_journal (wr, true);

   /* bulk insert some records to work on */
   {
      bson_init (&q);

      for (i = 0; i < 10; i++) {
         bson_init (&b[i]);
         bson_oid_init (&oid, NULL);
         bson_append_oid (&b[i], "_id", -1, &oid);
         bson_append_int32 (&b[i], "n", -1, i % 2);
         bptr[i] = &b[i];
      }

      BEGIN_IGNORE_DEPRECATIONS;
      ASSERT_OR_PRINT (mongoc_collection_insert_bulk (collection,
                                                      MONGOC_INSERT_NONE,
                                                      (const bson_t **) bptr,
                                                      10,
                                                      wr,
                                                      &error),
                       error);
      END_IGNORE_DEPRECATIONS;
   }

   /* create a couple of cursors */
   {
      cursor = mongoc_collection_find (
         collection, MONGOC_QUERY_EXHAUST, 0, 0, 0, &q, NULL, NULL);

      cursor2 = mongoc_collection_find (
         collection, MONGOC_QUERY_NONE, 0, 0, 0, &q, NULL, NULL);
   }

   /* Read from the exhaust cursor, ensure that we're in exhaust where we
    * should be and ensure that an early destroy properly causes a disconnect
    * */
   {
      r = mongoc_cursor_next (cursor, &doc);
      if (!r) {
         mongoc_cursor_error (cursor, &error);
         fprintf (stderr, "cursor error: %s\n", error.message);
      }
      BSON_ASSERT (r);
      BSON_ASSERT (doc);
      BSON_ASSERT (cursor->in_exhaust);
      BSON_ASSERT (client->in_exhaust);

      /* destroy the cursor, make sure a disconnect happened */
      timestamp1 = get_timestamp (client, cursor);
      mongoc_cursor_destroy (cursor);
      BSON_ASSERT (!client->in_exhaust);
   }

   /* ensure even a 1 ms-resolution clock advances significantly */
   _mongoc_usleep (1000 * 1000);

   /* Grab a new exhaust cursor, then verify that reading from that cursor
    * (putting the client into exhaust), breaks a mid-stream read from a
    * regular cursor */
   {
      cursor = mongoc_collection_find (
         collection, MONGOC_QUERY_EXHAUST, 0, 0, 0, &q, NULL, NULL);

      r = mongoc_cursor_next (cursor2, &doc);
      if (!r) {
         mongoc_cursor_error (cursor2, &error);
         fprintf (stderr, "cursor error: %s\n", error.message);
      }
      BSON_ASSERT (r);
      BSON_ASSERT (doc);
      ASSERT_CMPINT64 (timestamp1, <, get_timestamp (client, cursor2));

      for (i = 0; i < 5; i++) {
         r = mongoc_cursor_next (cursor2, &doc);
         if (!r) {
            mongoc_cursor_error (cursor2, &error);
            fprintf (stderr, "cursor error: %s\n", error.message);
         }
         BSON_ASSERT (r);
         BSON_ASSERT (doc);
      }

      r = mongoc_cursor_next (cursor, &doc);
      BSON_ASSERT (r);
      BSON_ASSERT (doc);

      doc = NULL;
      r = mongoc_cursor_next (cursor2, &doc);
      BSON_ASSERT (!r);
      BSON_ASSERT (!doc);

      mongoc_cursor_error (cursor2, &error);
      ASSERT_CMPUINT32 (error.domain, ==, MONGOC_ERROR_CLIENT);
      ASSERT_CMPUINT32 (error.code, ==, MONGOC_ERROR_CLIENT_IN_EXHAUST);

      mongoc_cursor_destroy (cursor2);
   }

   /* make sure writes fail as well */
   {
      BEGIN_IGNORE_DEPRECATIONS;
      r = mongoc_collection_insert_bulk (collection,
                                         MONGOC_INSERT_NONE,
                                         (const bson_t **) bptr,
                                         10,
                                         wr,
                                         &error);
      END_IGNORE_DEPRECATIONS;

      BSON_ASSERT (!r);
      ASSERT_CMPUINT32 (error.domain, ==, MONGOC_ERROR_CLIENT);
      ASSERT_CMPUINT32 (error.code, ==, MONGOC_ERROR_CLIENT_IN_EXHAUST);
   }

   /* we're still in exhaust.
    *
    * 1. check that we can create a new cursor, as long as we don't read from it
    * 2. fully exhaust the exhaust cursor
    * 3. make sure that we don't disconnect at destroy
    * 4. make sure we can read the cursor we made during the exhuast
    */
   {
      cursor2 = mongoc_collection_find (
         collection, MONGOC_QUERY_NONE, 0, 0, 0, &q, NULL, NULL);

      server_id = cursor->server_id;
      stream =
         (mongoc_stream_t *) mongoc_set_get (client->cluster.nodes, server_id);

      for (i = 1; i < 10; i++) {
         r = mongoc_cursor_next (cursor, &doc);
         BSON_ASSERT (r);
         BSON_ASSERT (doc);
      }

      r = mongoc_cursor_next (cursor, &doc);
      BSON_ASSERT (!r);
      BSON_ASSERT (!doc);

      mongoc_cursor_destroy (cursor);

      BSON_ASSERT (stream == (mongoc_stream_t *) mongoc_set_get (
                                client->cluster.nodes, server_id));

      r = mongoc_cursor_next (cursor2, &doc);
      BSON_ASSERT (r);
      BSON_ASSERT (doc);
   }

   bson_destroy (&q);
   for (i = 0; i < 10; i++) {
      bson_destroy (&b[i]);
   }

   ASSERT_OR_PRINT (mongoc_collection_drop (collection, &error), error);

   mongoc_write_concern_destroy (wr);
   mongoc_cursor_destroy (cursor2);
   mongoc_collection_destroy (collection);

   if (pooled) {
      mongoc_client_pool_push (pool, client);
      mongoc_client_pool_destroy (pool);
   } else {
      mongoc_client_destroy (client);
   }
}

static void
test_exhaust_cursor_single (void *context)
{
   test_exhaust_cursor (false);
}

static void
test_exhaust_cursor_pool (void *context)
{
   test_exhaust_cursor (true);
}

static void
test_exhaust_cursor_multi_batch (void *context)
{
   mongoc_client_t *client;
   bson_error_t error;
   mongoc_collection_t *collection;
   bson_t doc = BSON_INITIALIZER;
   mongoc_bulk_operation_t *bulk;
   int i;
   uint32_t server_id;
   mongoc_cursor_t *cursor;
   const bson_t *cursor_doc;

   client = test_framework_client_new ();
   collection = get_test_collection (client, "test_exhaust_cursor_multi_batch");

   ASSERT_OR_PRINT (collection, error);

   BSON_APPEND_UTF8 (&doc, "key", "value");
   bulk = mongoc_collection_create_bulk_operation (collection, true, NULL);

   /* enough to require more than initial batch */
   for (i = 0; i < 1000; i++) {
      mongoc_bulk_operation_insert (bulk, &doc);
   }

   server_id = mongoc_bulk_operation_execute (bulk, NULL, &error);
   ASSERT_OR_PRINT (server_id, error);

   cursor = mongoc_collection_find (
      collection, MONGOC_QUERY_EXHAUST, 0, 0, 0, tmp_bson ("{}"), NULL, NULL);

   i = 0;
   while (mongoc_cursor_next (cursor, &cursor_doc)) {
      i++;
      ASSERT (mongoc_cursor_is_alive (cursor));
   }

   ASSERT_OR_PRINT (!mongoc_cursor_error (cursor, &error), error);
   ASSERT (!mongoc_cursor_is_alive (cursor));
   ASSERT_CMPINT (i, ==, 1000);

   mongoc_cursor_destroy (cursor);
   mongoc_bulk_operation_destroy (bulk);
   mongoc_collection_destroy (collection);
   bson_destroy (&doc);
   mongoc_client_destroy (client);
}

static void
test_cursor_set_max_await_time_ms (void)
{
   mongoc_client_t *client;
   mongoc_collection_t *collection;
   mongoc_cursor_t *cursor;

   client = test_framework_client_new ();
   collection =
      get_test_collection (client, "test_cursor_set_max_await_time_ms");

   cursor = mongoc_collection_find (collection,
                                    MONGOC_QUERY_TAILABLE_CURSOR |
                                       MONGOC_QUERY_AWAIT_DATA,
                                    0,
                                    0,
                                    0,
                                    tmp_bson ("{}"),
                                    NULL,
                                    NULL);

   ASSERT_CMPINT (0, ==, mongoc_cursor_get_max_await_time_ms (cursor));
   mongoc_cursor_set_max_await_time_ms (cursor, 123);
   ASSERT_CMPINT (123, ==, mongoc_cursor_get_max_await_time_ms (cursor));
   _mongoc_cursor_next (cursor, NULL);

   /* once started, cursor ignores set_max_await_time_ms () */
   mongoc_cursor_set_max_await_time_ms (cursor, 42);
   ASSERT_CMPINT (123, ==, mongoc_cursor_get_max_await_time_ms (cursor));

   mongoc_cursor_destroy (cursor);
   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
}

typedef enum {
   FIRST_BATCH,
   SECOND_BATCH,
} exhaust_error_when_t;

typedef enum {
   NETWORK_ERROR,
   SERVER_ERROR,
} exhaust_error_type_t;

static void
_request_error (request_t *request, exhaust_error_type_t error_type)
{
   if (error_type == NETWORK_ERROR) {
      mock_server_resets (request);
   } else {
      mock_server_replies (request,
                           MONGOC_REPLY_QUERY_FAILURE,
                           123,
                           0,
                           0,
                           "{'$err': 'uh oh', 'code': 4321}");
   }
}

static void
_check_error (mongoc_client_t *client,
              mongoc_cursor_t *cursor,
              exhaust_error_type_t error_type)
{
   uint32_t server_id;
   bson_error_t error;

   server_id = mongoc_cursor_get_hint (cursor);
   ASSERT (server_id);
   ASSERT (mongoc_cursor_error (cursor, &error));

   if (error_type == NETWORK_ERROR) {
      ASSERT_ERROR_CONTAINS (error,
                             MONGOC_ERROR_STREAM,
                             MONGOC_ERROR_STREAM_SOCKET,
                             "socket error or timeout");

      /* socket was discarded */
      ASSERT (!mongoc_cluster_stream_for_server (
         &client->cluster, server_id, false /* don't reconnect */, &error));

      ASSERT_ERROR_CONTAINS (error,
                             MONGOC_ERROR_STREAM,
                             MONGOC_ERROR_STREAM_SOCKET,
                             "socket error or timeout");
   } else {
      /* query failure */
      ASSERT_ERROR_CONTAINS (error,
                             MONGOC_ERROR_QUERY,
                             4321 /* error from mock server */,
                             "uh oh" /* message from mock server */);
   }
}

static void
_mock_test_exhaust (bool pooled,
                    exhaust_error_when_t error_when,
                    exhaust_error_type_t error_type)
{
   mock_server_t *server;
   mongoc_client_pool_t *pool = NULL;
   mongoc_client_t *client;
   mongoc_collection_t *collection;
   mongoc_cursor_t *cursor;
   const bson_t *doc;
   future_t *future;
   request_t *request;

   capture_logs (true);

   server = mock_server_with_autoismaster (0);
   mock_server_run (server);

   if (pooled) {
      pool = mongoc_client_pool_new (mock_server_get_uri (server));
      client = mongoc_client_pool_pop (pool);
   } else {
      client = mongoc_client_new_from_uri (mock_server_get_uri (server));
   }

   collection = mongoc_client_get_collection (client, "db", "test");
   cursor = mongoc_collection_find (
      collection, MONGOC_QUERY_EXHAUST, 0, 0, 0, tmp_bson ("{}"), NULL, NULL);

   future = future_cursor_next (cursor, &doc);
   request =
      mock_server_receives_query (server,
                                  "db.test",
                                  MONGOC_QUERY_SLAVE_OK | MONGOC_QUERY_EXHAUST,
                                  0,
                                  0,
                                  "{}",
                                  NULL);

   if (error_when == SECOND_BATCH) {
      /* initial query succeeds, gets a doc and cursor id of 123 */
      mock_server_replies (request, MONGOC_REPLY_NONE, 123, 1, 1, "{'a': 1}");
      ASSERT (future_get_bool (future));
      ASSERT (match_bson (doc, tmp_bson ("{'a': 1}"), false));
      ASSERT_CMPINT64 ((int64_t) 123, ==, mongoc_cursor_get_id (cursor));

      future_destroy (future);

      /* error after initial batch */
      future = future_cursor_next (cursor, &doc);
   }

   _request_error (request, error_type);
   ASSERT (!future_get_bool (future));
   _check_error (client, cursor, error_type);

   future_destroy (future);
   request_destroy (request);
   mongoc_cursor_destroy (cursor);
   mongoc_collection_destroy (collection);

   if (pooled) {
      mongoc_client_pool_push (pool, client);
      mongoc_client_pool_destroy (pool);
   } else {
      mongoc_client_destroy (client);
   }

   mock_server_destroy (server);
}

static void
test_exhaust_network_err_1st_batch_single (void)
{
   _mock_test_exhaust (false, FIRST_BATCH, NETWORK_ERROR);
}

static void
test_exhaust_network_err_1st_batch_pooled (void)
{
   _mock_test_exhaust (true, FIRST_BATCH, NETWORK_ERROR);
}

static void
test_exhaust_server_err_1st_batch_single (void)
{
   _mock_test_exhaust (false, FIRST_BATCH, SERVER_ERROR);
}

static void
test_exhaust_server_err_1st_batch_pooled (void)
{
   _mock_test_exhaust (true, FIRST_BATCH, SERVER_ERROR);
}

static void
test_exhaust_network_err_2nd_batch_single (void)
{
   _mock_test_exhaust (false, SECOND_BATCH, NETWORK_ERROR);
}

static void
test_exhaust_network_err_2nd_batch_pooled (void)
{
   _mock_test_exhaust (true, SECOND_BATCH, NETWORK_ERROR);
}

static void
test_exhaust_server_err_2nd_batch_single (void)
{
   _mock_test_exhaust (false, SECOND_BATCH, SERVER_ERROR);
}

static void
test_exhaust_server_err_2nd_batch_pooled (void)
{
   _mock_test_exhaust (true, SECOND_BATCH, SERVER_ERROR);
}

void
test_exhaust_install (TestSuite *suite)
{
   TestSuite_AddFull (suite,
                      "/Client/exhaust_cursor/single",
                      test_exhaust_cursor_single,
                      NULL,
                      NULL,
                      skip_if_mongos);
   TestSuite_AddFull (suite,
                      "/Client/exhaust_cursor/pool",
                      test_exhaust_cursor_pool,
                      NULL,
                      NULL,
                      skip_if_mongos);
   TestSuite_AddFull (suite,
                      "/Client/exhaust_cursor/batches",
                      test_exhaust_cursor_multi_batch,
                      NULL,
                      NULL,
                      skip_if_mongos);
   TestSuite_AddLive (suite,
                      "/Client/set_max_await_time_ms",
                      test_cursor_set_max_await_time_ms);
   TestSuite_AddMockServerTest (
      suite,
      "/Client/exhaust_cursor/err/network/1st_batch/single",
      test_exhaust_network_err_1st_batch_single);
   TestSuite_AddMockServerTest (
      suite,
      "/Client/exhaust_cursor/err/network/1st_batch/pooled",
      test_exhaust_network_err_1st_batch_pooled);
   TestSuite_AddMockServerTest (
      suite,
      "/Client/exhaust_cursor/err/server/1st_batch/single",
      test_exhaust_server_err_1st_batch_single);
   TestSuite_AddMockServerTest (
      suite,
      "/Client/exhaust_cursor/err/server/1st_batch/pooled",
      test_exhaust_server_err_1st_batch_pooled);
   TestSuite_AddMockServerTest (
      suite,
      "/Client/exhaust_cursor/err/network/2nd_batch/single",
      test_exhaust_network_err_2nd_batch_single);
   TestSuite_AddMockServerTest (
      suite,
      "/Client/exhaust_cursor/err/network/2nd_batch/pooled",
      test_exhaust_network_err_2nd_batch_pooled);
   TestSuite_AddMockServerTest (
      suite,
      "/Client/exhaust_cursor/err/server/2nd_batch/single",
      test_exhaust_server_err_2nd_batch_single);
   TestSuite_AddMockServerTest (
      suite,
      "/Client/exhaust_cursor/err/server/2nd_batch/pooled",
      test_exhaust_server_err_2nd_batch_pooled);
}
