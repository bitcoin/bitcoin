#include <mongoc.h>
#include <mongoc-uri-private.h>

#include "mongoc-client-private.h"
#include "mongoc-util-private.h"
#include "TestSuite.h"

#include "test-libmongoc.h"
#include "mock_server/mock-server.h"
#include "mock_server/future.h"
#include "mock_server/future-functions.h"
#include "test-conveniences.h"

#undef MONGOC_LOG_DOMAIN
#define MONGOC_LOG_DOMAIN "topology-test"


static void
test_topology_client_creation (void)
{
   mongoc_uri_t *uri;
   mongoc_topology_scanner_node_t *node;
   mongoc_topology_t *topology_a;
   mongoc_topology_t *topology_b;
   mongoc_client_t *client_a;
   mongoc_client_t *client_b;
   mongoc_stream_t *topology_stream;
   mongoc_server_stream_t *server_stream;
   bson_error_t error;

   uri = test_framework_get_uri ();
   mongoc_uri_set_option_as_int32 (uri, "localThresholdMS", 42);
   mongoc_uri_set_option_as_int32 (uri, "connectTimeoutMS", 12345);
   mongoc_uri_set_option_as_int32 (uri, "serverSelectionTimeoutMS", 54321);

   /* create two clients directly */
   client_a = mongoc_client_new_from_uri (uri);
   client_b = mongoc_client_new_from_uri (uri);
   BSON_ASSERT (client_a);
   BSON_ASSERT (client_b);

#ifdef MONGOC_ENABLE_SSL
   test_framework_set_ssl_opts (client_a);
   test_framework_set_ssl_opts (client_b);
#endif

   /* ensure that they are using different topologies */
   topology_a = client_a->topology;
   topology_b = client_b->topology;
   BSON_ASSERT (topology_a);
   BSON_ASSERT (topology_b);
   BSON_ASSERT (topology_a != topology_b);

   BSON_ASSERT (topology_a->local_threshold_msec == 42);
   BSON_ASSERT (topology_a->connect_timeout_msec == 12345);
   BSON_ASSERT (topology_a->server_selection_timeout_msec == 54321);

   /* ensure that their topologies are running in single-threaded mode */
   BSON_ASSERT (topology_a->single_threaded);
   BSON_ASSERT (topology_a->scanner_state == MONGOC_TOPOLOGY_SCANNER_OFF);

   /* ensure that we are sharing streams with the client */
   server_stream =
      mongoc_cluster_stream_for_reads (&client_a->cluster, NULL, &error);

   ASSERT_OR_PRINT (server_stream, error);
   node = mongoc_topology_scanner_get_node (client_a->topology->scanner,
                                            server_stream->sd->id);
   BSON_ASSERT (node);
   topology_stream = node->stream;
   BSON_ASSERT (topology_stream);
   BSON_ASSERT (topology_stream == server_stream->stream);

   mongoc_server_stream_cleanup (server_stream);
   mongoc_client_destroy (client_a);
   mongoc_client_destroy (client_b);
   mongoc_uri_destroy (uri);
}

static void
test_topology_client_pool_creation (void)
{
   mongoc_client_pool_t *pool;
   mongoc_client_t *client_a;
   mongoc_client_t *client_b;
   mongoc_topology_t *topology_a;
   mongoc_topology_t *topology_b;

   /* create two clients through a client pool */
   pool = test_framework_client_pool_new ();
   client_a = mongoc_client_pool_pop (pool);
   client_b = mongoc_client_pool_pop (pool);
   BSON_ASSERT (client_a);
   BSON_ASSERT (client_b);

   /* ensure that they are using the same topology */
   topology_a = client_a->topology;
   topology_b = client_b->topology;
   BSON_ASSERT (topology_a);
   BSON_ASSERT (topology_a == topology_b);

   /* ensure that that topology is running in a background thread */
   BSON_ASSERT (!topology_a->single_threaded);
   BSON_ASSERT (topology_a->scanner_state != MONGOC_TOPOLOGY_SCANNER_OFF);

   mongoc_client_pool_push (pool, client_a);
   mongoc_client_pool_push (pool, client_b);
   mongoc_client_pool_destroy (pool);
}

static void
test_server_selection_try_once_option (void *ctx)
{
   const char *uri_strings[3] = {"mongodb://a",
                                 "mongodb://a/?serverSelectionTryOnce=true",
                                 "mongodb://a/?serverSelectionTryOnce=false"};

   unsigned long i;
   mongoc_client_t *client;
   mongoc_uri_t *uri;
   mongoc_client_pool_t *pool;

   /* try_once is on by default for non-pooled, can be turned off */
   client = mongoc_client_new (uri_strings[0]);
   BSON_ASSERT (client->topology->server_selection_try_once);
   mongoc_client_destroy (client);

   client = mongoc_client_new (uri_strings[1]);
   BSON_ASSERT (client->topology->server_selection_try_once);
   mongoc_client_destroy (client);

   client = mongoc_client_new (uri_strings[2]);
   BSON_ASSERT (!client->topology->server_selection_try_once);
   mongoc_client_destroy (client);

   /* off for pooled clients, can't be enabled */
   for (i = 0; i < sizeof (uri_strings) / sizeof (char *); i++) {
      uri = mongoc_uri_new ("mongodb://a");
      pool = mongoc_client_pool_new (uri);
      client = mongoc_client_pool_pop (pool);
      BSON_ASSERT (!client->topology->server_selection_try_once);
      mongoc_client_pool_push (pool, client);
      mongoc_client_pool_destroy (pool);
      mongoc_uri_destroy (uri);
   }
}

static void
_test_server_selection (bool try_once)
{
   mock_server_t *server;
   char *secondary_response;
   char *primary_response;
   mongoc_uri_t *uri;
   mongoc_client_t *client;
   mongoc_read_prefs_t *primary_pref;
   future_t *future;
   bson_error_t error;
   request_t *request;
   mongoc_server_description_t *sd;

   if (!TestSuite_CheckMockServerAllowed ()) {
      return;
   }

   server = mock_server_new ();
   mock_server_run (server);

   secondary_response =
      bson_strdup_printf ("{'ok': 1, "
                          " 'ismaster': false,"
                          " 'secondary': true,"
                          " 'setName': 'rs',"
                          " 'hosts': ['%s']}",
                          mock_server_get_host_and_port (server));

   primary_response =
      bson_strdup_printf ("{'ok': 1, "
                          " 'ismaster': true,"
                          " 'setName': 'rs',"
                          " 'hosts': ['%s']}",
                          mock_server_get_host_and_port (server));

   uri = mongoc_uri_copy (mock_server_get_uri (server));
   mongoc_uri_set_option_as_utf8 (uri, "replicaSet", "rs");
   mongoc_uri_set_option_as_int32 (uri, "heartbeatFrequencyMS", 500);
   mongoc_uri_set_option_as_int32 (uri, "serverSelectionTimeoutMS", 100);
   if (!try_once) {
      /* serverSelectionTryOnce is on by default */
      mongoc_uri_set_option_as_bool (uri, "serverSelectionTryOnce", false);
   }

   client = mongoc_client_new_from_uri (uri);
   primary_pref = mongoc_read_prefs_new (MONGOC_READ_PRIMARY);

   /* no primary, selection fails after one try */
   future = future_topology_select (
      client->topology, MONGOC_SS_READ, primary_pref, &error);
   request = mock_server_receives_ismaster (server);
   BSON_ASSERT (request);
   mock_server_replies_simple (request, secondary_response);
   request_destroy (request);

   /* the selection timeout is 100 ms, and we can't rescan until a half second
    * passes, so selection fails without another ismaster call */
   mock_server_set_request_timeout_msec (server, 600);
   BSON_ASSERT (!mock_server_receives_ismaster (server));
   mock_server_set_request_timeout_msec (server, get_future_timeout_ms ());

   /* selection fails */
   BSON_ASSERT (!future_get_mongoc_server_description_ptr (future));
   ASSERT_CMPINT (error.domain, ==, MONGOC_ERROR_SERVER_SELECTION);
   ASSERT_CMPINT (error.code, ==, MONGOC_ERROR_SERVER_SELECTION_FAILURE);
   ASSERT_STARTSWITH (error.message, "No suitable servers found");

   if (try_once) {
      ASSERT_CONTAINS (error.message, "serverSelectionTryOnce");
   } else {
      ASSERT_CONTAINS (error.message, "serverselectiontimeoutms");
   }

   BSON_ASSERT (client->topology->stale);
   future_destroy (future);

   _mongoc_usleep (510 * 1000); /* one heartbeat, plus a few milliseconds */

   /* second selection, now we try ismaster again */
   future = future_topology_select (
      client->topology, MONGOC_SS_READ, primary_pref, &error);
   request = mock_server_receives_ismaster (server);
   BSON_ASSERT (request);

   /* the secondary is now primary, selection succeeds */
   mock_server_replies_simple (request, primary_response);
   sd = future_get_mongoc_server_description_ptr (future);
   BSON_ASSERT (sd);
   BSON_ASSERT (!client->topology->stale);
   request_destroy (request);
   future_destroy (future);

   mongoc_server_description_destroy (sd);
   mongoc_read_prefs_destroy (primary_pref);
   mongoc_client_destroy (client);
   mongoc_uri_destroy (uri);
   bson_free (secondary_response);
   bson_free (primary_response);
   mock_server_destroy (server);
}

static void
test_server_selection_try_once (void *ctx)
{
   _test_server_selection (true);
}

static void
test_server_selection_try_once_false (void *ctx)
{
   _test_server_selection (false);
}

static void
host_list_init (mongoc_host_list_t *host_list,
                int family,
                const char *host,
                uint16_t port)
{
   memset (host_list, 0, sizeof *host_list);
   host_list->family = family;
   bson_snprintf (host_list->host, sizeof host_list->host, "%s", host);
   bson_snprintf (host_list->host_and_port,
                  sizeof host_list->host_and_port,
                  "%s:%hu",
                  host,
                  port);
}

static void
_test_topology_invalidate_server (bool pooled)
{
   mongoc_server_description_t *fake_sd;
   mongoc_server_description_t *sd;
   mongoc_topology_description_t *td;
   mongoc_client_t *client;
   mongoc_client_pool_t *pool = NULL;
   bson_error_t error;
   mongoc_host_list_t fake_host_list;
   uint32_t fake_id = 42;
   uint32_t id;
   mongoc_server_stream_t *server_stream;

   if (pooled) {
      pool = test_framework_client_pool_new ();
      client = mongoc_client_pool_pop (pool);

      /* background scanner complains about failed connection */
      capture_logs (true);
   } else {
      client = test_framework_client_new ();
   }

   td = &client->topology->description;

   /* call explicitly */
   server_stream =
      mongoc_cluster_stream_for_reads (&client->cluster, NULL, &error);
   ASSERT_OR_PRINT (server_stream, error);
   id = server_stream->sd->id;
   sd = (mongoc_server_description_t *) mongoc_set_get (td->servers, id);
   BSON_ASSERT (sd);
   BSON_ASSERT (sd->type == MONGOC_SERVER_STANDALONE ||
                sd->type == MONGOC_SERVER_RS_PRIMARY ||
                sd->type == MONGOC_SERVER_MONGOS);

   ASSERT_CMPINT64 (sd->round_trip_time_msec, !=, (int64_t) -1);

   bson_set_error (
      &error, MONGOC_ERROR_STREAM, MONGOC_ERROR_STREAM_SOCKET, "error");
   mongoc_topology_invalidate_server (client->topology, id, &error);
   sd = (mongoc_server_description_t *) mongoc_set_get (td->servers, id);
   BSON_ASSERT (sd);
   BSON_ASSERT (sd->type == MONGOC_SERVER_UNKNOWN);
   ASSERT_CMPINT64 (sd->round_trip_time_msec, ==, (int64_t) -1);

   fake_sd = (mongoc_server_description_t *) bson_malloc0 (sizeof (*fake_sd));

   /* insert a 'fake' server description and ensure that it is invalidated by
    * driver */
   host_list_init (&fake_host_list, AF_INET, "fakeaddress", 27033);
   mongoc_server_description_init (
      fake_sd, fake_host_list.host_and_port, fake_id);

   fake_sd->type = MONGOC_SERVER_STANDALONE;
   mongoc_set_add (td->servers, fake_id, fake_sd);
   mongoc_topology_scanner_add (
      client->topology->scanner, &fake_host_list, fake_id);
   BSON_ASSERT (!mongoc_cluster_stream_for_server (
      &client->cluster, fake_id, true, &error));
   sd = (mongoc_server_description_t *) mongoc_set_get (td->servers, fake_id);
   BSON_ASSERT (sd);
   BSON_ASSERT (sd->type == MONGOC_SERVER_UNKNOWN);
   BSON_ASSERT (sd->error.domain != 0);
   ASSERT_CMPINT64 (sd->round_trip_time_msec, ==, (int64_t) -1);

   mongoc_server_stream_cleanup (server_stream);

   if (pooled) {
      mongoc_client_pool_push (pool, client);
      mongoc_client_pool_destroy (pool);
   } else {
      mongoc_client_destroy (client);
   }
}

static void
test_topology_invalidate_server_single (void)
{
   _test_topology_invalidate_server (false);
}

static void
test_topology_invalidate_server_pooled (void)
{
   _test_topology_invalidate_server (true);
}

static void
test_invalid_cluster_node (void *ctx)
{
   mongoc_client_pool_t *pool;
   mongoc_cluster_node_t *cluster_node;
   mongoc_topology_scanner_node_t *scanner_node;
   bson_error_t error;
   mongoc_client_t *client;
   mongoc_cluster_t *cluster;
   mongoc_server_stream_t *server_stream;
   int64_t scanner_node_ts;
   uint32_t id;

   /* use client pool, this test is only valid when multi-threaded */
   pool = test_framework_client_pool_new ();
   client = mongoc_client_pool_pop (pool);
   cluster = &client->cluster;

   _mongoc_usleep (100 * 1000);

   /* load stream into cluster */
   server_stream =
      mongoc_cluster_stream_for_reads (&client->cluster, NULL, &error);
   ASSERT_OR_PRINT (server_stream, error);
   id = server_stream->sd->id;
   mongoc_server_stream_cleanup (server_stream);

   cluster_node = (mongoc_cluster_node_t *) mongoc_set_get (cluster->nodes, id);
   BSON_ASSERT (cluster_node);
   BSON_ASSERT (cluster_node->stream);

   mongoc_mutex_lock (&client->topology->mutex);
   scanner_node =
      mongoc_topology_scanner_get_node (client->topology->scanner, id);
   BSON_ASSERT (scanner_node);
   ASSERT_CMPINT64 (cluster_node->timestamp, >, scanner_node->timestamp);

   /* update the scanner node's timestamp */
   _mongoc_usleep (1000 * 1000);
   scanner_node_ts = scanner_node->timestamp = bson_get_monotonic_time ();
   ASSERT_CMPINT64 (cluster_node->timestamp, <, scanner_node_ts);
   _mongoc_usleep (1000 * 1000);
   mongoc_mutex_unlock (&client->topology->mutex);

   /* cluster discards node and creates new one */
   server_stream =
      mongoc_cluster_stream_for_server (&client->cluster, id, true, &error);
   ASSERT_OR_PRINT (server_stream, error);
   cluster_node = (mongoc_cluster_node_t *) mongoc_set_get (cluster->nodes, id);
   ASSERT_CMPINT64 (cluster_node->timestamp, >, scanner_node_ts);

   mongoc_server_stream_cleanup (server_stream);
   mongoc_client_pool_push (pool, client);
   mongoc_client_pool_destroy (pool);
}

static void
test_max_wire_version_race_condition (void *ctx)
{
   mongoc_topology_scanner_node_t *scanner_node;
   mongoc_server_description_t *sd;
   mongoc_database_t *database;
   mongoc_client_pool_t *pool;
   mongoc_client_t *client;
   bson_error_t error;
   mongoc_server_stream_t *server_stream;
   uint32_t id;

   /* connect directly and add our user, test is only valid with auth */
   client = test_framework_client_new ();
   database = mongoc_client_get_database (client, "test");
   mongoc_database_remove_user (database, "pink", &error);
   ASSERT_OR_PRINT (1 == mongoc_database_add_user (
                            database, "pink", "panther", NULL, NULL, &error),
                    error);
   mongoc_database_destroy (database);
   mongoc_client_destroy (client);

   /* use client pool, test is only valid when multi-threaded */
   pool = test_framework_client_pool_new ();
   client = mongoc_client_pool_pop (pool);

   /* load stream into cluster */
   server_stream =
      mongoc_cluster_stream_for_reads (&client->cluster, NULL, &error);
   ASSERT_OR_PRINT (server_stream, error);
   id = server_stream->sd->id;
   mongoc_server_stream_cleanup (server_stream);

   /* "disconnect": invalidate timestamp and reset server description */
   scanner_node =
      mongoc_topology_scanner_get_node (client->topology->scanner, id);
   BSON_ASSERT (scanner_node);
   scanner_node->timestamp = bson_get_monotonic_time ();
   sd = (mongoc_server_description_t *) mongoc_set_get (
      client->topology->description.servers, id);
   BSON_ASSERT (sd);
   mongoc_server_description_reset (sd);

   /* new stream, ensure that we can still auth with cached wire version */
   server_stream =
      mongoc_cluster_stream_for_server (&client->cluster, id, true, &error);
   ASSERT_OR_PRINT (server_stream, error);
   BSON_ASSERT (server_stream);

   mongoc_server_stream_cleanup (server_stream);
   mongoc_client_pool_push (pool, client);
   mongoc_client_pool_destroy (pool);
}


static void
test_cooldown_standalone (void *ctx)
{
   mock_server_t *server;
   mongoc_uri_t *uri;
   mongoc_client_t *client;
   mongoc_read_prefs_t *primary_pref;
   future_t *future;
   bson_error_t error;
   request_t *request;
   mongoc_server_description_t *sd;

   if (!TestSuite_CheckMockServerAllowed ()) {
      return;
   }

   server = mock_server_new ();
   mock_server_run (server);
   uri = mongoc_uri_copy (mock_server_get_uri (server));
   /* anything less than minHeartbeatFrequencyMS=500 is irrelevant */
   mongoc_uri_set_option_as_int32 (uri, "serverSelectionTimeoutMS", 100);
   client = mongoc_client_new_from_uri (uri);
   primary_pref = mongoc_read_prefs_new (MONGOC_READ_PRIMARY);

   /* first ismaster fails, selection fails */
   future = future_topology_select (
      client->topology, MONGOC_SS_READ, primary_pref, &error);
   request = mock_server_receives_ismaster (server);
   BSON_ASSERT (request);
   mock_server_hangs_up (request);
   BSON_ASSERT (!future_get_mongoc_server_description_ptr (future));
   request_destroy (request);
   future_destroy (future);

   _mongoc_usleep (1000 * 1000); /* 1 second */

   /* second selection doesn't try to call ismaster: we're in cooldown */
   future = future_topology_select (
      client->topology, MONGOC_SS_READ, primary_pref, &error);
   mock_server_set_request_timeout_msec (server, 100);
   BSON_ASSERT (!mock_server_receives_ismaster (server)); /* no ismaster call */
   BSON_ASSERT (!future_get_mongoc_server_description_ptr (future));
   future_destroy (future);
   mock_server_set_request_timeout_msec (server, get_future_timeout_ms ());

   _mongoc_usleep (5100 * 1000); /* 5.1 seconds */

   /* cooldown ends, now we try ismaster again, this time succeeding */
   future = future_topology_select (
      client->topology, MONGOC_SS_READ, primary_pref, &error);
   request = mock_server_receives_ismaster (server); /* not in cooldown now */
   BSON_ASSERT (request);
   mock_server_replies_simple (request, "{'ok': 1, 'ismaster': true}");
   sd = future_get_mongoc_server_description_ptr (future);
   BSON_ASSERT (sd);
   request_destroy (request);
   future_destroy (future);

   mongoc_server_description_destroy (sd);
   mongoc_read_prefs_destroy (primary_pref);
   mongoc_client_destroy (client);
   mongoc_uri_destroy (uri);
   mock_server_destroy (server);
}


static void
test_cooldown_rs (void *ctx)
{
   mock_server_t *servers[2]; /* two secondaries, no primary */
   char *uri_str;
   int i;
   mongoc_client_t *client;
   mongoc_read_prefs_t *primary_pref;
   char *secondary_response;
   char *primary_response;
   future_t *future;
   bson_error_t error;
   request_t *request;
   mongoc_server_description_t *sd;

   if (!TestSuite_CheckMockServerAllowed ()) {
      return;
   }

   for (i = 0; i < 2; i++) {
      servers[i] = mock_server_new ();
      mock_server_run (servers[i]);
   }

   uri_str = bson_strdup_printf ("mongodb://localhost:%hu/?replicaSet=rs"
                                 "&serverSelectionTimeoutMS=100"
                                 "&connectTimeoutMS=100",
                                 mock_server_get_port (servers[0]));

   client = mongoc_client_new (uri_str);
   primary_pref = mongoc_read_prefs_new (MONGOC_READ_PRIMARY);

   secondary_response = bson_strdup_printf (
      "{'ok': 1, 'ismaster': false, 'secondary': true, 'setName': 'rs',"
      " 'hosts': ['localhost:%hu', 'localhost:%hu']}",
      mock_server_get_port (servers[0]),
      mock_server_get_port (servers[1]));

   primary_response =
      bson_strdup_printf ("{'ok': 1, 'ismaster': true, 'setName': 'rs',"
                          " 'hosts': ['localhost:%hu']}",
                          mock_server_get_port (servers[1]));

   /* server 0 is a secondary. */
   future = future_topology_select (
      client->topology, MONGOC_SS_READ, primary_pref, &error);

   request = mock_server_receives_ismaster (servers[0]);
   BSON_ASSERT (request);
   mock_server_replies_simple (request, secondary_response);
   request_destroy (request);

   /* server 0 told us about server 1. we check it immediately but it's down. */
   request = mock_server_receives_ismaster (servers[1]);
   BSON_ASSERT (request);
   mock_server_hangs_up (request);
   request_destroy (request);

   /* selection fails. */
   BSON_ASSERT (!future_get_mongoc_server_description_ptr (future));
   future_destroy (future);

   _mongoc_usleep (1000 * 1000); /* 1 second */

   /* second selection doesn't try ismaster on server 1: it's in cooldown */
   future = future_topology_select (
      client->topology, MONGOC_SS_READ, primary_pref, &error);

   request = mock_server_receives_ismaster (servers[0]);
   BSON_ASSERT (request);
   mock_server_replies_simple (request, secondary_response);
   request_destroy (request);

   mock_server_set_request_timeout_msec (servers[1], 100);
   BSON_ASSERT (
      !mock_server_receives_ismaster (servers[1])); /* no ismaster call */
   mock_server_set_request_timeout_msec (servers[1], get_future_timeout_ms ());

   /* still no primary */
   BSON_ASSERT (!future_get_mongoc_server_description_ptr (future));
   future_destroy (future);

   _mongoc_usleep (5100 * 1000); /* 5.1 seconds. longer than 5 sec cooldown. */

   /* cooldown ends, now we try ismaster on server 1, this time succeeding */
   future = future_topology_select (
      client->topology, MONGOC_SS_READ, primary_pref, &error);

   request = mock_server_receives_ismaster (servers[1]);
   BSON_ASSERT (request);
   mock_server_replies_simple (request, primary_response);
   request_destroy (request);

   /* server 0 doesn't need to respond */
   sd = future_get_mongoc_server_description_ptr (future);
   BSON_ASSERT (sd);
   future_destroy (future);

   mongoc_server_description_destroy (sd);
   mongoc_read_prefs_destroy (primary_pref);
   mongoc_client_destroy (client);
   bson_free (secondary_response);
   bson_free (primary_response);
   bson_free (uri_str);
   mock_server_destroy (servers[0]);
   mock_server_destroy (servers[1]);
}


static void
_test_select_succeed (bool try_once)
{
   const int32_t connect_timeout_ms = 200;

   mock_server_t *primary;
   mock_server_t *secondary;
   mongoc_server_description_t *sd;
   char *uri_str;
   mongoc_uri_t *uri;
   mongoc_client_t *client;
   future_t *future;
   int64_t start;
   bson_error_t error;
   int64_t duration_usec;

   primary = mock_server_new ();
   mock_server_run (primary);

   secondary = mock_server_new ();
   mock_server_run (secondary);

   /* primary auto-responds, secondary never responds */
   mock_server_auto_ismaster (primary,
                              "{'ok': 1,"
                              " 'ismaster': true,"
                              " 'setName': 'rs',"
                              " 'hosts': ['localhost:%hu', 'localhost:%hu']}",
                              mock_server_get_port (primary),
                              mock_server_get_port (secondary));

   uri_str = bson_strdup_printf ("mongodb://localhost:%hu,localhost:%hu/"
                                 "?replicaSet=rs&connectTimeoutMS=%d",
                                 mock_server_get_port (primary),
                                 mock_server_get_port (secondary),
                                 connect_timeout_ms);

   uri = mongoc_uri_new (uri_str);
   BSON_ASSERT (uri);
   if (!try_once) {
      /* override default */
      mongoc_uri_set_option_as_bool (uri, "serverSelectionTryOnce", false);
   }

   client = mongoc_client_new_from_uri (uri);

   /* start waiting for a primary (NULL read pref) */
   start = bson_get_monotonic_time ();
   future =
      future_topology_select (client->topology, MONGOC_SS_READ, NULL, &error);

   /* selection succeeds */
   sd = future_get_mongoc_server_description_ptr (future);
   ASSERT_OR_PRINT (sd, error);
   future_destroy (future);

   duration_usec = bson_get_monotonic_time () - start;

   if (!test_suite_valgrind ()) {
      ASSERT_ALMOST_EQUAL (duration_usec / 1000, connect_timeout_ms);
   }

   mongoc_client_destroy (client);
   mongoc_uri_destroy (uri);
   bson_free (uri_str);
   mongoc_server_description_destroy (sd);
   mock_server_destroy (primary);
   mock_server_destroy (secondary);
}


/* CDRIVER-1219: a secondary is unavailable, scan should take connectTimeoutMS,
 * then we select primary */
static void
test_select_after_timeout (void)
{
   _test_select_succeed (false);
}


/* CDRIVER-1219: a secondary is unavailable, scan should try it once,
 * then we select primary */
static void
test_select_after_try_once (void)
{
   _test_select_succeed (true);
}


static void
test_multiple_selection_errors (void *context)
{
   const char *uri = "mongodb://doesntexist,example.com:2/?replicaSet=rs"
                     "&connectTimeoutMS=100";
   mongoc_client_t *client;
   bson_t reply;
   bson_error_t error;

   client = mongoc_client_new (uri);
   mongoc_client_command_simple (
      client, "test", tmp_bson ("{'ping': 1}"), NULL, &reply, &error);

   ASSERT_CMPINT (MONGOC_ERROR_SERVER_SELECTION, ==, error.domain);
   ASSERT_CMPINT (MONGOC_ERROR_SERVER_SELECTION_FAILURE, ==, error.code);

   /* Like:
    * "No suitable servers found (`serverselectiontryonce` set):
    *  [Failed to resolve 'doesntexist']
    *  [connection error calling ismaster on 'example.com:2']"
    */
   ASSERT_CONTAINS (error.message, "No suitable servers found");
   /* either "connection error" or "connection timeout" calling ismaster */
   ASSERT_CONTAINS (error.message, "calling ismaster on 'example.com:2'");
   ASSERT_CONTAINS (error.message, "[Failed to resolve 'doesntexist']");

   mongoc_client_destroy (client);
}


static void
test_invalid_server_id (void)
{
   mongoc_client_t *client;
   bson_error_t error;

   client = test_framework_client_new ();

   BSON_ASSERT (
      !mongoc_topology_server_by_id (client->topology, 99999, &error));
   ASSERT_STARTSWITH (error.message, "Could not find description for node");

   mongoc_client_destroy (client);
}


static bool
auto_ping (request_t *request, void *data)
{
   if (!request->is_command || strcasecmp (request->command_name, "ping")) {
      return false;
   }

   mock_server_replies_ok_and_destroys (request);

   return true;
}


/* Tests CDRIVER-562: after calling ismaster to handshake a new connection we
 * must update topology description with the server response.
 */
static void
_test_server_removed_during_handshake (bool pooled)
{
   mock_server_t *server;
   mongoc_uri_t *uri;
   mongoc_client_pool_t *pool = NULL;
   mongoc_client_t *client;
   bool r;
   bson_error_t error;
   mongoc_server_description_t *sd;
   mongoc_server_description_t **sds;
   size_t n;

   server = mock_server_new ();
   mock_server_run (server);
   mock_server_autoresponds (server, auto_ping, NULL, NULL);
   mock_server_auto_ismaster (server,
                              "{'ok': 1,"
                              " 'ismaster': true,"
                              " 'setName': 'rs',"
                              " 'hosts': ['%s']}",
                              mock_server_get_host_and_port (server));

   uri = mongoc_uri_copy (mock_server_get_uri (server));
   /* no auto heartbeat */
   mongoc_uri_set_option_as_int32 (uri, "heartbeatFrequencyMS", INT32_MAX);
   mongoc_uri_set_option_as_utf8 (uri, "replicaSet", "rs");

   if (pooled) {
      pool = mongoc_client_pool_new (uri);
      client = mongoc_client_pool_pop (pool);
   } else {
      client = mongoc_client_new_from_uri (uri);
   }

   /* initial connection, discover one-node replica set */
   r = mongoc_client_command_simple (
      client, "db", tmp_bson ("{'ping': 1}"), NULL, NULL, &error);

   ASSERT_OR_PRINT (r, error);

   ASSERT_CMPINT (_mongoc_topology_get_type (client->topology),
                  ==,
                  MONGOC_TOPOLOGY_RS_WITH_PRIMARY);
   sd = mongoc_client_get_server_description (client, 1);
   ASSERT_CMPINT ((int) MONGOC_SERVER_RS_PRIMARY, ==, sd->type);
   mongoc_server_description_destroy (sd);

   /* primary changes setName */
   mock_server_auto_ismaster (server,
                              "{'ok': 1,"
                              " 'ismaster': true,"
                              " 'setName': 'BAD NAME',"
                              " 'hosts': ['%s']}",
                              mock_server_get_host_and_port (server));

   /* pretend to close a connection. does NOT affect server description yet */
   mongoc_cluster_disconnect_node (&client->cluster, 1);
   sd = mongoc_client_get_server_description (client, 1);
   /* still primary */
   ASSERT_CMPINT ((int) MONGOC_SERVER_RS_PRIMARY, ==, sd->type);
   mongoc_server_description_destroy (sd);

   /* opens new stream and runs ismaster again, discovers bad setName. */
   r = mongoc_client_command_simple (
      client, "db", tmp_bson ("{'ping': 1}"), NULL, NULL, &error);

   ASSERT (!r);
   ASSERT_ERROR_CONTAINS (error,
                          MONGOC_ERROR_STREAM,
                          MONGOC_ERROR_STREAM_NOT_ESTABLISHED,
                          "removed from topology");

   sds = mongoc_client_get_server_descriptions (client, &n);
   ASSERT_CMPSIZE_T (n, ==, (size_t) 0);
   ASSERT_CMPINT (_mongoc_topology_get_type (client->topology),
                  ==,
                  MONGOC_TOPOLOGY_RS_NO_PRIMARY);

   if (pooled) {
      mongoc_client_pool_push (pool, client);
      mongoc_client_pool_destroy (pool);
   } else {
      mongoc_client_destroy (client);
   }

   mongoc_server_descriptions_destroy_all (sds, n);
   mock_server_destroy (server);
   mongoc_uri_destroy (uri);
}


static void
test_server_removed_during_handshake_single (void)
{
   _test_server_removed_during_handshake (false);
}


static void
test_server_removed_during_handshake_pooled (void)
{
   _test_server_removed_during_handshake (true);
}


static void
test_rtt (void *ctx)
{
   mock_server_t *server;
   mongoc_client_t *client;
   future_t *future;
   request_t *request;
   bson_error_t error;
   mongoc_server_description_t *sd;
   int64_t rtt_msec;

   if (!TestSuite_CheckMockServerAllowed ()) {
      return;
   }

   server = mock_server_new ();
   mock_server_run (server);

   client = mongoc_client_new_from_uri (mock_server_get_uri (server));
   future = future_client_command_simple (
      client, "db", tmp_bson ("{'ping': 1}"), NULL, NULL, &error);

   request = mock_server_receives_ismaster (server);
   _mongoc_usleep (1000 * 1000); /* one second */
   mock_server_replies_ok_and_destroys (request);
   request = mock_server_receives_command (
      server, "db", MONGOC_QUERY_SLAVE_OK, "{'ping': 1}");
   mock_server_replies_ok_and_destroys (request);
   ASSERT_OR_PRINT (future_get_bool (future), error);

   sd = mongoc_topology_server_by_id (client->topology, 1, NULL);
   ASSERT (sd);

   /* assert, with plenty of slack, that rtt was calculated in ms, not usec */
   rtt_msec = mongoc_server_description_round_trip_time (sd);
   ASSERT_CMPINT64 (rtt_msec, >, (int64_t) 900);  /* 900 ms */
   ASSERT_CMPINT64 (rtt_msec, <, (int64_t) 9000); /* 9 seconds */

   mongoc_server_description_destroy (sd);
   future_destroy (future);
   mongoc_client_destroy (client);
   mock_server_destroy (server);
}


/* mongoc_topology_scanner_add and mongoc_topology_scan are called within the
 * topology mutex to add a discovered node and call getaddrinfo on its host
 * immediately - test that this doesn't cause a recursive acquire on the
 * topology mutex */
static void
test_add_and_scan_failure (void)
{
   mock_server_t *server;
   mongoc_uri_t *uri;
   mongoc_client_pool_t *pool;
   mongoc_client_t *client;
   future_t *future;
   request_t *request;
   bson_error_t error;
   mongoc_server_description_t *sd;

   server = mock_server_new ();
   mock_server_run (server);
   /* client will discover "fake" host and fail to connect */
   mock_server_auto_ismaster (server,
                              "{'ok': 1,"
                              " 'ismaster': true,"
                              " 'setName': 'rs',"
                              " 'hosts': ['%s', 'fake:1']}",
                              mock_server_get_host_and_port (server));

   uri = mongoc_uri_copy (mock_server_get_uri (server));
   mongoc_uri_set_option_as_utf8 (uri, "replicaSet", "rs");
   pool = mongoc_client_pool_new (uri);
   client = mongoc_client_pool_pop (pool);
   future = future_client_command_simple (
      client, "db", tmp_bson ("{'ping': 1}"), NULL, NULL, &error);

   request = mock_server_receives_command (
      server, "db", MONGOC_QUERY_NONE, "{'ping': 1}");
   mock_server_replies_ok_and_destroys (request);
   ASSERT_OR_PRINT (future_get_bool (future), error);

   sd = mongoc_topology_server_by_id (client->topology, 1, NULL);
   ASSERT (sd);
   ASSERT_CMPSTR (mongoc_server_description_type (sd), "RSPrimary");
   mongoc_server_description_destroy (sd);

   sd = mongoc_topology_server_by_id (client->topology, 2, NULL);
   ASSERT (sd);
   ASSERT_CMPSTR (mongoc_server_description_type (sd), "Unknown");
   mongoc_server_description_destroy (sd);

   future_destroy (future);
   mongoc_client_pool_push (pool, client);
   mongoc_client_pool_destroy (pool);
   mongoc_uri_destroy (uri);
   mock_server_destroy (server);
}


typedef struct {
   int n_started;
   int n_succeeded;
   int n_failed;
} checks_t;


static void
check_started (const mongoc_apm_server_heartbeat_started_t *event)
{
   checks_t *c;

   c = (checks_t *) mongoc_apm_server_heartbeat_started_get_context (event);
   c->n_started++;
}


static void
check_succeeded (const mongoc_apm_server_heartbeat_succeeded_t *event)
{
   checks_t *c;

   c = (checks_t *) mongoc_apm_server_heartbeat_succeeded_get_context (event);
   c->n_succeeded++;
}


static void
check_failed (const mongoc_apm_server_heartbeat_failed_t *event)
{
   checks_t *c;

   c = (checks_t *) mongoc_apm_server_heartbeat_failed_get_context (event);
   c->n_failed++;
}


static mongoc_apm_callbacks_t *
heartbeat_callbacks (void)
{
   mongoc_apm_callbacks_t *callbacks;

   callbacks = mongoc_apm_callbacks_new ();
   mongoc_apm_set_server_heartbeat_started_cb (callbacks, check_started);
   mongoc_apm_set_server_heartbeat_succeeded_cb (callbacks, check_succeeded);
   mongoc_apm_set_server_heartbeat_failed_cb (callbacks, check_failed);

   return callbacks;
}


static future_t *
future_command (mongoc_client_t *client, bson_error_t *error)
{
   return future_client_command_simple (
      client, "admin", tmp_bson ("{'foo': 1}"), NULL, NULL, error);
}


static void
receives_command (mock_server_t *server, future_t *future)
{
   request_t *request;
   bson_error_t error;

   request = mock_server_receives_command (
      server, "admin", MONGOC_QUERY_NONE, "{'foo': 1}");
   mock_server_replies_ok_and_destroys (request);
   ASSERT_OR_PRINT (future_get_bool (future), error);
   future_destroy (future);
}


static bool
has_known_server (mongoc_client_t *client)
{
   mongoc_server_description_t *sd;
   bool r;

   /* in this test we know the server id is always 1 */
   sd = mongoc_client_get_server_description (client, 1);
   r = (sd->type != MONGOC_SERVER_UNKNOWN);
   mongoc_server_description_destroy (sd);
   return r;
}


static void
_test_ismaster_retry_single (bool hangup, int n_failures)
{
   checks_t checks = {0};
   mongoc_apm_callbacks_t *callbacks;
   mock_server_t *server;
   mongoc_uri_t *uri;
   mongoc_client_t *client;
   char *ismaster;
   future_t *future;
   request_t *request;
   bson_error_t error;
   int64_t t;

   server = mock_server_new ();
   mock_server_run (server);
   uri = mongoc_uri_copy (mock_server_get_uri (server));
   mongoc_uri_set_option_as_int32 (uri, MONGOC_URI_HEARTBEATFREQUENCYMS, 500);
   mongoc_uri_set_option_as_utf8 (uri, MONGOC_URI_REPLICASET, "rs");
   if (!hangup) {
      mongoc_uri_set_option_as_int32 (uri, MONGOC_URI_CONNECTTIMEOUTMS, 100);
   }

   client = mongoc_client_new_from_uri (uri);
   callbacks = heartbeat_callbacks ();
   mongoc_client_set_apm_callbacks (client, callbacks, &checks);

   ismaster = bson_strdup_printf ("{'ok': 1,"
                                  " 'ismaster': true,"
                                  " 'setName': 'rs',"
                                  " 'hosts': ['%s']}",
                                  mock_server_get_host_and_port (server));

   /* start a {foo: 1} command, handshake normally */
   future = future_command (client, &error);
   request = mock_server_receives_ismaster (server);
   mock_server_replies_simple (request, ismaster);
   request_destroy (request);
   receives_command (server, future);

   /* wait for the next server check */
   _mongoc_usleep (600 * 1000);

   /* start a {foo: 1} command, server check fails and retries immediately */
   future = future_command (client, &error);
   request = mock_server_receives_ismaster (server);
   t = bson_get_monotonic_time ();
   if (hangup) {
      mock_server_hangs_up (request);
   }

   request_destroy (request);

   /* retry immediately (for testing, "immediately" means less than 250ms */
   request = mock_server_receives_ismaster (server);
   ASSERT_CMPINT64 (bson_get_monotonic_time () - t, <, (int64_t) 250 * 1000);

   if (n_failures == 2) {
      if (hangup) {
         mock_server_hangs_up (request);
      }

      BSON_ASSERT (!future_get_bool (future));
      future_destroy (future);
   } else {
      mock_server_replies_simple (request, ismaster);
      /* the {foo: 1} command finishes */
      receives_command (server, future);
   }

   request_destroy (request);

   ASSERT_CMPINT (checks.n_started, ==, 3);
   WAIT_UNTIL (checks.n_succeeded == 3 - n_failures);
   WAIT_UNTIL (checks.n_failed == n_failures);

   if (n_failures == 2) {
      BSON_ASSERT (!has_known_server (client));
   } else {
      BSON_ASSERT (has_known_server (client));
   }

   mongoc_client_destroy (client);
   mongoc_uri_destroy (uri);
   mock_server_destroy (server);
   bson_free (ismaster);
   mongoc_apm_callbacks_destroy (callbacks);
}


static void
_test_ismaster_retry_pooled (bool hangup, int n_failures)
{
   checks_t checks = {0};
   mongoc_apm_callbacks_t *callbacks;
   mock_server_t *server;
   mongoc_uri_t *uri;
   mongoc_client_pool_t *pool = NULL;
   mongoc_client_t *client;
   char *ismaster;
   future_t *future;
   request_t *request;
   bson_error_t error;
   int i;
   int64_t t;

   server = mock_server_new ();
   mock_server_run (server);
   uri = mongoc_uri_copy (mock_server_get_uri (server));
   mongoc_uri_set_option_as_int32 (uri, MONGOC_URI_HEARTBEATFREQUENCYMS, 500);
   mongoc_uri_set_option_as_utf8 (uri, MONGOC_URI_REPLICASET, "rs");
   if (!hangup) {
      mongoc_uri_set_option_as_int32 (uri, MONGOC_URI_CONNECTTIMEOUTMS, 100);
   }

   pool = mongoc_client_pool_new (uri);
   callbacks = heartbeat_callbacks ();
   mongoc_client_pool_set_apm_callbacks (pool, callbacks, &checks);
   client = mongoc_client_pool_pop (pool);

   ismaster = bson_strdup_printf ("{'ok': 1,"
                                  " 'ismaster': true,"
                                  " 'setName': 'rs',"
                                  " 'hosts': ['%s']}",
                                  mock_server_get_host_and_port (server));

   /* start a {foo: 1} command, handshake normally */
   future = future_command (client, &error);

   /* one ismaster from the scanner, another to handshake the connection */
   for (i = 0; i < 2; i++) {
      request = mock_server_receives_ismaster (server);
      mock_server_replies_simple (request, ismaster);
      request_destroy (request);
   }

   /* the {foo: 1} command finishes */
   receives_command (server, future);

   /* wait for the next server check */
   request = mock_server_receives_ismaster (server);
   t = bson_get_monotonic_time ();
   if (hangup) {
      mock_server_hangs_up (request);
   }

   request_destroy (request);

   /* retry immediately (for testing, "immediately" means less than 250ms */
   request = mock_server_receives_ismaster (server);
   ASSERT_CMPINT64 (bson_get_monotonic_time () - t, <, (int64_t) 250 * 1000);
   if (n_failures == 2) {
      if (hangup) {
         mock_server_hangs_up (request);
      }
      BSON_ASSERT (!has_known_server (client));
   } else {
      mock_server_replies_simple (request, ismaster);
      WAIT_UNTIL (has_known_server (client));
   }

   request_destroy (request);

   WAIT_UNTIL (checks.n_succeeded == 3 - n_failures);
   WAIT_UNTIL (checks.n_failed == n_failures);
   ASSERT_CMPINT (checks.n_started, ==, 3);

   mongoc_client_pool_push (pool, client);
   mongoc_client_pool_destroy (pool);
   mongoc_uri_destroy (uri);
   mock_server_destroy (server);
   bson_free (ismaster);
   mongoc_apm_callbacks_destroy (callbacks);
}


static void
test_ismaster_retry_single_hangup (void)
{
   _test_ismaster_retry_single (true, 1);
}


static void
test_ismaster_retry_single_timeout (void)
{
   _test_ismaster_retry_single (false, 1);
}

static void
test_ismaster_retry_single_hangup_fail (void)
{
   _test_ismaster_retry_single (true, 2);
}


static void
test_ismaster_retry_single_timeout_fail (void)
{
   _test_ismaster_retry_single (false, 2);
}


static void
test_ismaster_retry_pooled_hangup (void)
{
   _test_ismaster_retry_pooled (true, 1);
}


static void
test_ismaster_retry_pooled_timeout (void)
{
   _test_ismaster_retry_pooled (false, 1);
}


static void
test_ismaster_retry_pooled_hangup_fail (void)
{
   _test_ismaster_retry_pooled (true, 2);
}


static void
test_ismaster_retry_pooled_timeout_fail (void)
{
   _test_ismaster_retry_pooled (false, 2);
}


void
test_topology_install (TestSuite *suite)
{
   TestSuite_AddLive (
      suite, "/Topology/client_creation", test_topology_client_creation);
   TestSuite_AddLive (suite,
                      "/Topology/client_pool_creation",
                      test_topology_client_pool_creation);
   TestSuite_AddFull (suite,
                      "/Topology/server_selection_try_once_option",
                      test_server_selection_try_once_option,
                      NULL,
                      NULL,
                      test_framework_skip_if_slow);
   TestSuite_AddFull (suite,
                      "/Topology/server_selection_try_once",
                      test_server_selection_try_once,
                      NULL,
                      NULL,
                      test_framework_skip_if_slow);
   TestSuite_AddFull (suite,
                      "/Topology/server_selection_try_once_false",
                      test_server_selection_try_once_false,
                      NULL,
                      NULL,
                      test_framework_skip_if_slow);
   TestSuite_AddLive (suite,
                      "/Topology/invalidate_server/single",
                      test_topology_invalidate_server_single);
   TestSuite_AddLive (suite,
                      "/Topology/invalidate_server/pooled",
                      test_topology_invalidate_server_pooled);
   TestSuite_AddFull (suite,
                      "/Topology/invalid_cluster_node",
                      test_invalid_cluster_node,
                      NULL,
                      NULL,
                      test_framework_skip_if_slow_or_live);
   TestSuite_AddFull (suite,
                      "/Topology/max_wire_version_race_condition",
                      test_max_wire_version_race_condition,
                      NULL,
                      NULL,
                      test_framework_skip_if_no_auth);
   TestSuite_AddFull (suite,
                      "/Topology/cooldown/standalone",
                      test_cooldown_standalone,
                      NULL,
                      NULL,
                      test_framework_skip_if_slow);
   TestSuite_AddFull (suite,
                      "/Topology/cooldown/rs",
                      test_cooldown_rs,
                      NULL,
                      NULL,
                      test_framework_skip_if_slow);
   TestSuite_AddFull (suite,
                      "/Topology/multiple_selection_errors",
                      test_multiple_selection_errors,
                      NULL,
                      NULL,
                      test_framework_skip_if_offline);
   TestSuite_AddMockServerTest (
      suite, "/Topology/connect_timeout/succeed", test_select_after_timeout);
   TestSuite_AddMockServerTest (
      suite, "/Topology/try_once/succeed", test_select_after_try_once);
   TestSuite_AddLive (
      suite, "/Topology/invalid_server_id", test_invalid_server_id);
   TestSuite_AddMockServerTest (suite,
                                "/Topology/server_removed/single",
                                test_server_removed_during_handshake_single);
   TestSuite_AddMockServerTest (suite,
                                "/Topology/server_removed/pooled",
                                test_server_removed_during_handshake_pooled);
   TestSuite_AddFull (suite,
                      "/Topology/rtt",
                      test_rtt,
                      NULL,
                      NULL,
                      test_framework_skip_if_slow);
   TestSuite_AddMockServerTest (
      suite, "/Topology/add_and_scan_failure", test_add_and_scan_failure);
   TestSuite_AddMockServerTest (
      suite, "/Topology/ismaster_retry/single/hangup", test_ismaster_retry_single_hangup);
   TestSuite_AddMockServerTest (
      suite, "/Topology/ismaster_retry/single/timeout", test_ismaster_retry_single_timeout);
   TestSuite_AddMockServerTest (
      suite, "/Topology/ismaster_retry/single/hangup/fail", test_ismaster_retry_single_hangup_fail);
   TestSuite_AddMockServerTest (
      suite, "/Topology/ismaster_retry/single/timeout/fail", test_ismaster_retry_single_timeout_fail);
   TestSuite_AddMockServerTest (
      suite, "/Topology/ismaster_retry/pooled/hangup", test_ismaster_retry_pooled_hangup);
   TestSuite_AddMockServerTest (
      suite, "/Topology/ismaster_retry/pooled/timeout", test_ismaster_retry_pooled_timeout);
   TestSuite_AddMockServerTest (
      suite, "/Topology/ismaster_retry/pooled/hangup/fail", test_ismaster_retry_pooled_hangup_fail);
   TestSuite_AddMockServerTest (
      suite, "/Topology/ismaster_retry/pooled/timeout/fail", test_ismaster_retry_pooled_timeout_fail);
}
