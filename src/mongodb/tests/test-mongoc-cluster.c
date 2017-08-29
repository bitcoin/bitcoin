#include <mongoc-util-private.h>
#include <mongoc.h>

#include "mongoc-client-private.h"
#include "mongoc-uri-private.h"

#include "mock_server/mock-server.h"
#include "mock_server/future.h"
#include "mock_server/future-functions.h"
#include "TestSuite.h"
#include "test-libmongoc.h"
#include "test-conveniences.h"


#undef MONGOC_LOG_DOMAIN
#define MONGOC_LOG_DOMAIN "cluster-test"


static uint32_t
server_id_for_reads (mongoc_cluster_t *cluster)
{
   bson_error_t error;
   mongoc_server_stream_t *server_stream;
   uint32_t id;

   server_stream = mongoc_cluster_stream_for_reads (cluster, NULL, &error);
   ASSERT_OR_PRINT (server_stream, error);
   id = server_stream->sd->id;

   mongoc_server_stream_cleanup (server_stream);

   return id;
}


static void
test_get_max_bson_obj_size (void)
{
   mongoc_server_description_t *sd;
   mongoc_cluster_node_t *node;
   mongoc_client_pool_t *pool;
   mongoc_client_t *client;
   int32_t max_bson_obj_size = 16;
   uint32_t id;

   /* single-threaded */
   client = test_framework_client_new ();
   BSON_ASSERT (client);

   id = server_id_for_reads (&client->cluster);
   sd = (mongoc_server_description_t *) mongoc_set_get (
      client->topology->description.servers, id);
   sd->max_bson_obj_size = max_bson_obj_size;
   BSON_ASSERT (max_bson_obj_size ==
                mongoc_cluster_get_max_bson_obj_size (&client->cluster));

   mongoc_client_destroy (client);

   /* multi-threaded */
   pool = test_framework_client_pool_new ();
   client = mongoc_client_pool_pop (pool);

   id = server_id_for_reads (&client->cluster);
   node = (mongoc_cluster_node_t *) mongoc_set_get (client->cluster.nodes, id);
   node->max_bson_obj_size = max_bson_obj_size;
   BSON_ASSERT (max_bson_obj_size ==
                mongoc_cluster_get_max_bson_obj_size (&client->cluster));

   mongoc_client_pool_push (pool, client);
   mongoc_client_pool_destroy (pool);
}

static void
test_get_max_msg_size (void)
{
   mongoc_server_description_t *sd;
   mongoc_cluster_node_t *node;
   mongoc_client_pool_t *pool;
   mongoc_client_t *client;
   int32_t max_msg_size = 32;
   uint32_t id;

   /* single-threaded */
   client = test_framework_client_new ();
   id = server_id_for_reads (&client->cluster);

   sd = (mongoc_server_description_t *) mongoc_set_get (
      client->topology->description.servers, id);
   sd->max_msg_size = max_msg_size;
   BSON_ASSERT (max_msg_size ==
                mongoc_cluster_get_max_msg_size (&client->cluster));

   mongoc_client_destroy (client);

   /* multi-threaded */
   pool = test_framework_client_pool_new ();
   client = mongoc_client_pool_pop (pool);

   id = server_id_for_reads (&client->cluster);
   node = (mongoc_cluster_node_t *) mongoc_set_get (client->cluster.nodes, id);
   node->max_msg_size = max_msg_size;
   BSON_ASSERT (max_msg_size ==
                mongoc_cluster_get_max_msg_size (&client->cluster));

   mongoc_client_pool_push (pool, client);
   mongoc_client_pool_destroy (pool);
}


#define ASSERT_CURSOR_ERR()                                  \
   do {                                                      \
      BSON_ASSERT (!future_get_bool (future));               \
      BSON_ASSERT (mongoc_cursor_error (cursor, &error));    \
      ASSERT_ERROR_CONTAINS (                                \
         error,                                              \
         MONGOC_ERROR_STREAM,                                \
         MONGOC_ERROR_STREAM_SOCKET,                         \
         "Failed to read 4 bytes: socket error or timeout"); \
   } while (0)


#define START_QUERY(client_port_variable)                               \
   do {                                                                 \
      cursor = mongoc_collection_find_with_opts (                       \
         collection, tmp_bson ("{}"), NULL, NULL);                      \
      future = future_cursor_next (cursor, &doc);                       \
      request = mock_server_receives_query (                            \
         server, "test.test", MONGOC_QUERY_SLAVE_OK, 0, 0, "{}", NULL); \
      client_port_variable = request_get_client_port (request);         \
   } while (0)


#define CLEANUP_QUERY()               \
   do {                               \
      request_destroy (request);      \
      future_destroy (future);        \
      mongoc_cursor_destroy (cursor); \
   } while (0)


/* test that we reconnect a cluster node after disconnect */
static void
_test_cluster_node_disconnect (bool pooled)
{
   mock_server_t *server;
   const int32_t socket_timeout_ms = 100;
   mongoc_uri_t *uri;
   mongoc_client_pool_t *pool = NULL;
   mongoc_client_t *client;
   mongoc_collection_t *collection;
   const bson_t *doc;
   mongoc_cursor_t *cursor;
   future_t *future;
   request_t *request;
   uint16_t client_port_0, client_port_1;
   bson_error_t error;

   if (!TestSuite_CheckMockServerAllowed ()) {
      return;
   }

   capture_logs (true);

   server = mock_server_with_autoismaster (0);
   mock_server_run (server);

   uri = mongoc_uri_copy (mock_server_get_uri (server));
   mongoc_uri_set_option_as_int32 (uri, "socketTimeoutMS", socket_timeout_ms);

   if (pooled) {
      pool = mongoc_client_pool_new (uri);
      client = mongoc_client_pool_pop (pool);
   } else {
      client = mongoc_client_new_from_uri (uri);
   }

   collection = mongoc_client_get_collection (client, "test", "test");

   /* query 0 fails. set client_port_0 to the port used by the query. */
   START_QUERY (client_port_0);

   mock_server_resets (request);
   ASSERT_CURSOR_ERR ();
   CLEANUP_QUERY ();

   /* query 1 opens a new socket. set client_port_1 to the new port. */
   START_QUERY (client_port_1);
   ASSERT_CMPINT (client_port_1, !=, client_port_0);
   mock_server_replies_simple (request, "{'a': 1}");

   /* success! */
   BSON_ASSERT (future_get_bool (future));

   CLEANUP_QUERY ();
   mongoc_collection_destroy (collection);

   if (pooled) {
      mongoc_client_pool_push (pool, client);
      mongoc_client_pool_destroy (pool);
   } else {
      mongoc_client_destroy (client);
   }

   mongoc_uri_destroy (uri);
   mock_server_destroy (server);
}


static void
test_cluster_node_disconnect_single (void *ctx)
{
   _test_cluster_node_disconnect (false);
}


static void
test_cluster_node_disconnect_pooled (void *ctx)
{
   _test_cluster_node_disconnect (true);
}


static void
_test_cluster_command_timeout (bool pooled)
{
   mock_server_t *server;
   mongoc_uri_t *uri;
   mongoc_client_pool_t *pool = NULL;
   mongoc_client_t *client;
   bson_error_t error;
   future_t *future;
   request_t *request;
   uint16_t client_port;
   bson_t reply;

   capture_logs (true);

   server = mock_server_with_autoismaster (0);
   mock_server_run (server);
   uri = mongoc_uri_copy (mock_server_get_uri (server));
   mongoc_uri_set_option_as_int32 (uri, "socketTimeoutMS", 200);

   if (pooled) {
      pool = mongoc_client_pool_new (uri);
      client = mongoc_client_pool_pop (pool);
   } else {
      client = mongoc_client_new_from_uri (uri);
   }

   /* server doesn't respond in time */
   future = future_client_command_simple (
      client, "db", tmp_bson ("{'foo': 1}"), NULL, NULL, &error);
   request =
      mock_server_receives_command (server, "db", MONGOC_QUERY_SLAVE_OK, NULL);
   client_port = request_get_client_port (request);

   ASSERT (!future_get_bool (future));
   ASSERT_ERROR_CONTAINS (
      error,
      MONGOC_ERROR_STREAM,
      MONGOC_ERROR_STREAM_SOCKET,
      "Failed to send \"foo\" command with database \"db\"");

   /* late response */
   mock_server_replies_simple (request, "{'ok': 1, 'bar': 1}");
   request_destroy (request);
   future_destroy (future);

   future = future_client_command_simple (
      client, "db", tmp_bson ("{'baz': 1}"), NULL, &reply, &error);
   request = mock_server_receives_command (
      server, "db", MONGOC_QUERY_SLAVE_OK, "{'baz': 1}");
   ASSERT (request);
   /* new socket */
   ASSERT_CMPUINT16 (client_port, !=, request_get_client_port (request));
   mock_server_replies_simple (request, "{'ok': 1, 'quux': 1}");
   ASSERT (future_get_bool (future));

   /* got the proper response */
   ASSERT_HAS_FIELD (&reply, "quux");

   if (pooled) {
      mongoc_client_pool_push (pool, client);
      mongoc_client_pool_destroy (pool);
   } else {
      mongoc_client_destroy (client);
   }

   bson_destroy (&reply);
   request_destroy (request);
   future_destroy (future);
   mongoc_uri_destroy (uri);
   mock_server_destroy (server);
}


static void
test_cluster_command_timeout_single (void)
{
   _test_cluster_command_timeout (false);
}


static void
test_cluster_command_timeout_pooled (void)
{
   _test_cluster_command_timeout (true);
}


static void
_test_write_disconnect (bool legacy)
{
   mock_server_t *server;
   char *ismaster_response;
   mongoc_client_t *client;
   mongoc_collection_t *collection;
   bson_error_t error;
   future_t *future;
   request_t *request;
   mongoc_topology_scanner_node_t *scanner_node;

   if (!TestSuite_CheckMockServerAllowed ()) {
      return;
   }

   server = mock_server_new ();
   mock_server_run (server);
   client = mongoc_client_new_from_uri (mock_server_get_uri (server));

   /*
    * establish connection with an "ismaster" and "ping"
    */
   future = future_client_command_simple (
      client, "db", tmp_bson ("{'ping': 1}"), NULL, NULL, &error);
   request = mock_server_receives_ismaster (server);
   ismaster_response = bson_strdup_printf ("{'ok': 1.0,"
                                           " 'ismaster': true,"
                                           " 'minWireVersion': 0,"
                                           " 'maxWireVersion': %d}",
                                           legacy ? 0 : 3);

   mock_server_replies_simple (request, ismaster_response);
   request_destroy (request);

   request = mock_server_receives_command (
      server, "db", MONGOC_QUERY_SLAVE_OK, "{'ping': 1}");
   mock_server_replies_simple (request, "{'ok': 1}");
   ASSERT_OR_PRINT (future_get_bool (future), error);

   /*
    * close the socket
    */
   mock_server_hangs_up (request);

   /*
    * next operation detects the hangup
    */
   collection = mongoc_client_get_collection (client, "db", "collection");
   future_destroy (future);
   future = future_collection_insert (
      collection, MONGOC_INSERT_NONE, tmp_bson ("{'_id': 1}"), NULL, &error);

   ASSERT (!future_get_bool (future));
   ASSERT_CMPINT (error.domain, ==, MONGOC_ERROR_STREAM);
   ASSERT_CMPINT (error.code, ==, MONGOC_ERROR_STREAM_SOCKET);

   scanner_node = mongoc_topology_scanner_get_node (client->topology->scanner,
                                                    1 /* server_id */);
   ASSERT (scanner_node && !scanner_node->stream);

   mongoc_collection_destroy (collection);
   request_destroy (request);
   future_destroy (future);
   bson_free (ismaster_response);
   mongoc_client_destroy (client);
   mock_server_destroy (server);
}


static void
test_write_command_disconnect (void *ctx)
{
   _test_write_disconnect (false);
}


static void
test_legacy_write_disconnect (void *ctx)
{
   _test_write_disconnect (true);
}


void
test_cluster_install (TestSuite *suite)
{
   TestSuite_AddLive (
      suite, "/Cluster/test_get_max_bson_obj_size", test_get_max_bson_obj_size);
   TestSuite_AddLive (
      suite, "/Cluster/test_get_max_msg_size", test_get_max_msg_size);
   TestSuite_AddFull (suite,
                      "/Cluster/disconnect/single",
                      test_cluster_node_disconnect_single,
                      NULL,
                      NULL,
                      test_framework_skip_if_slow);
   TestSuite_AddFull (suite,
                      "/Cluster/disconnect/pooled",
                      test_cluster_node_disconnect_pooled,
                      NULL,
                      NULL,
                      test_framework_skip_if_slow);
   TestSuite_AddMockServerTest (suite,
                                "/Cluster/command/timeout/single",
                                test_cluster_command_timeout_single);
   TestSuite_AddMockServerTest (suite,
                                "/Cluster/command/timeout/pooled",
                                test_cluster_command_timeout_pooled);
   TestSuite_AddFull (suite,
                      "/Cluster/write_command/disconnect",
                      test_write_command_disconnect,
                      NULL,
                      NULL,
                      test_framework_skip_if_slow);
   TestSuite_AddFull (suite,
                      "/Cluster/legacy_write/disconnect",
                      test_legacy_write_disconnect,
                      NULL,
                      NULL,
                      test_framework_skip_if_slow);
}
