#include <mongoc.h>
#include <mongoc-util-private.h>

#include "mongoc-client-private.h"

#include "TestSuite.h"
#include "json-test.h"
#include "test-libmongoc.h"
#include "mock_server/mock-server.h"
#include "mock_server/future-functions.h"

#undef MONGOC_LOG_DOMAIN
#define MONGOC_LOG_DOMAIN "client-test-max-staleness"


static int64_t
get_max_staleness (const mongoc_client_t *client)
{
   const mongoc_read_prefs_t *prefs;

   prefs = mongoc_client_get_read_prefs (client);

   return mongoc_read_prefs_get_max_staleness_seconds (prefs);
}


/* the next few tests are from max-staleness-tests.rst */
static void
test_mongoc_client_max_staleness (void)
{
   mongoc_client_t *client;

   client = mongoc_client_new (NULL);
   ASSERT_CMPINT64 (get_max_staleness (client), ==, (int64_t) -1);
   mongoc_client_destroy (client);

   client = mongoc_client_new ("mongodb://a/?" MONGOC_URI_READPREFERENCE
                               "=secondary");
   ASSERT_CMPINT64 (get_max_staleness (client), ==, (int64_t) -1);
   mongoc_client_destroy (client);

   /* -1 is the default, means "no max staleness" */
   client =
      mongoc_client_new ("mongodb://a/?" MONGOC_URI_MAXSTALENESSSECONDS "=-1");
   ASSERT_CMPINT64 (get_max_staleness (client), ==, (int64_t) -1);
   mongoc_client_destroy (client);

   client =
      mongoc_client_new ("mongodb://a/?" MONGOC_URI_READPREFERENCE
                         "=primary&" MONGOC_URI_MAXSTALENESSSECONDS "=-1");

   ASSERT_CMPINT64 (get_max_staleness (client), ==, (int64_t) -1);
   mongoc_client_destroy (client);

   /* no " MONGOC_URI_MAXSTALENESSSECONDS " with primary mode */
   capture_logs (true);
   ASSERT (!mongoc_client_new ("mongodb://a/?" MONGOC_URI_MAXSTALENESSSECONDS
                               "=120"));
   ASSERT (!mongoc_client_new ("mongodb://a/?" MONGOC_URI_READPREFERENCE
                               "=primary&" MONGOC_URI_MAXSTALENESSSECONDS
                               "=120"));
   ASSERT_CAPTURED_LOG (MONGOC_URI_MAXSTALENESSSECONDS "=120",
                        MONGOC_LOG_LEVEL_WARNING,
                        "Invalid readPreferences");

   capture_logs (true);

   /* zero is prohibited */
   client = mongoc_client_new ("mongodb://a/?" MONGOC_URI_READPREFERENCE
                               "=nearest&" MONGOC_URI_MAXSTALENESSSECONDS "=0");

   ASSERT_CAPTURED_LOG (
      MONGOC_URI_MAXSTALENESSSECONDS "=0",
      MONGOC_LOG_LEVEL_WARNING,
      "Unsupported value for \"" MONGOC_URI_MAXSTALENESSSECONDS "\": \"0\"");

   ASSERT_CMPINT64 (get_max_staleness (client), ==, (int64_t) -1);
   mongoc_client_destroy (client);

   client = mongoc_client_new ("mongodb://a/?" MONGOC_URI_MAXSTALENESSSECONDS
                               "=120&" MONGOC_URI_READPREFERENCE "=secondary");

   ASSERT_CMPINT64 (get_max_staleness (client), ==, (int64_t) 120);
   mongoc_client_destroy (client);

   /* float is ignored */
   capture_logs (true);
   ASSERT (!mongoc_client_new ("mongodb://a/?" MONGOC_URI_READPREFERENCE
                               "=secondary&" MONGOC_URI_MAXSTALENESSSECONDS
                               "=10.5"));

   ASSERT_CAPTURED_LOG (MONGOC_URI_MAXSTALENESSSECONDS "=10.5",
                        MONGOC_LOG_LEVEL_WARNING,
                        "Invalid " MONGOC_URI_MAXSTALENESSSECONDS);

   /* 1 is allowed, it'll be rejected once we begin server selection */
   client =
      mongoc_client_new ("mongodb://a/?" MONGOC_URI_READPREFERENCE
                         "=secondary&" MONGOC_URI_MAXSTALENESSSECONDS "=1");

   ASSERT_EQUAL_DOUBLE (get_max_staleness (client), 1);
   mongoc_client_destroy (client);
}


static void
test_mongos_max_staleness_read_pref (void)
{
   mock_server_t *server;
   mongoc_collection_t *collection;
   mongoc_client_t *client;
   mongoc_read_prefs_t *prefs;
   future_t *future;
   request_t *request;
   bson_error_t error;

   server = mock_mongos_new (5 /* maxWireVersion */);
   mock_server_run (server);
   client = mongoc_client_new_from_uri (mock_server_get_uri (server));
   collection = mongoc_client_get_collection (client, "db", "collection");

   /* count command with mode "secondary", no " MONGOC_URI_MAXSTALENESSSECONDS "
    */
   prefs = mongoc_read_prefs_new (MONGOC_READ_SECONDARY);
   mongoc_collection_set_read_prefs (collection, prefs);
   future = future_collection_count (
      collection, MONGOC_QUERY_NONE, NULL, 0, 0, NULL, &error);
   request = mock_server_receives_command (
      server,
      "db",
      MONGOC_QUERY_SLAVE_OK,
      "{'$" MONGOC_URI_READPREFERENCE "': {'mode': 'secondary', "
      "                     'maxStalenessSeconds': {'$exists': false}}}");

   mock_server_replies_simple (request, "{'ok': 1, 'n': 1}");
   ASSERT_OR_PRINT (1 == future_get_int64_t (future), error);

   request_destroy (request);
   future_destroy (future);

   /* count command with mode "secondary". " MONGOC_URI_MAXSTALENESSSECONDS "=1
    * is allowed by
    * client, although in real life mongos will reject it */
   mongoc_read_prefs_set_max_staleness_seconds (prefs, 1);
   mongoc_collection_set_read_prefs (collection, prefs);

   mongoc_collection_set_read_prefs (collection, prefs);
   future = future_collection_count (
      collection, MONGOC_QUERY_NONE, NULL, 0, 0, NULL, &error);
   request = mock_server_receives_command (
      server,
      "db",
      MONGOC_QUERY_SLAVE_OK,
      "{'$readPreference': {'mode': 'secondary', 'maxStalenessSeconds': 1}}",
      NULL);

   mock_server_replies_simple (request, "{'ok': 1, 'n': 1}");
   ASSERT_OR_PRINT (1 == future_get_int64_t (future), error);

   request_destroy (request);
   future_destroy (future);

   mongoc_read_prefs_destroy (prefs);
   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
   mock_server_destroy (server);
}


static void
_test_last_write_date (bool pooled)
{
   mongoc_uri_t *uri;
   mongoc_client_pool_t *pool = NULL;
   mongoc_client_t *client;
   mongoc_collection_t *collection;
   bson_error_t error;
   bool r;
   mongoc_server_description_t *s0, *s1;
   int64_t delta;

   uri = test_framework_get_uri ();
   mongoc_uri_set_option_as_int32 (uri, "heartbeatFrequencyMS", 500);

   if (pooled) {
      pool = mongoc_client_pool_new (uri);
      test_framework_set_pool_ssl_opts (pool);
      client = mongoc_client_pool_pop (pool);
   } else {
      client = mongoc_client_new_from_uri (uri);
      test_framework_set_ssl_opts (client);
   }

   collection = get_test_collection (client, "test_last_write_date");
   r = mongoc_collection_insert (
      collection, MONGOC_INSERT_NONE, tmp_bson ("{}"), NULL, &error);
   ASSERT_OR_PRINT (r, error);

   _mongoc_usleep (1000 * 1000);
   s0 = mongoc_topology_select (client->topology, MONGOC_SS_READ, NULL, &error);
   ASSERT_OR_PRINT (s0, error);

   r = mongoc_collection_insert (
      collection, MONGOC_INSERT_NONE, tmp_bson ("{}"), NULL, &error);
   ASSERT_OR_PRINT (r, error);

   _mongoc_usleep (1000 * 1000);
   s1 = mongoc_topology_select (client->topology, MONGOC_SS_READ, NULL, &error);
   ASSERT_OR_PRINT (s1, error);
   ASSERT_CMPINT64 (s1->last_write_date_ms, !=, (int64_t) -1);

   /* lastWriteDate increased by roughly one second - be lenient, just check
    * it increased by less than 10 seconds */
   delta = s1->last_write_date_ms - s0->last_write_date_ms;
   ASSERT_CMPINT64 (delta, >, (int64_t) 0);
   ASSERT_CMPINT64 (delta, <, (int64_t) 10 * 1000);

   mongoc_server_description_destroy (s0);
   mongoc_server_description_destroy (s1);
   mongoc_collection_destroy (collection);

   if (pooled) {
      mongoc_client_pool_push (pool, client);
      mongoc_client_pool_destroy (pool);
   } else {
      mongoc_client_destroy (client);
   }
}


static void
test_last_write_date (void *ctx)
{
   _test_last_write_date (false);
}


static void
test_last_write_date_pooled (void *ctx)
{
   _test_last_write_date (true);
}


/* run only if wire version is older than 5 */
static void
_test_last_write_date_absent (bool pooled)
{
   mongoc_client_pool_t *pool = NULL;
   mongoc_client_t *client;
   bson_error_t error;
   mongoc_server_description_t *sd;

   if (pooled) {
      pool = test_framework_client_pool_new ();
      client = mongoc_client_pool_pop (pool);
   } else {
      client = test_framework_client_new ();
   }

   sd = mongoc_topology_select (client->topology, MONGOC_SS_READ, NULL, &error);
   ASSERT_OR_PRINT (sd, error);

   /* lastWriteDate absent */
   ASSERT_CMPINT64 (sd->last_write_date_ms, ==, (int64_t) -1);

   mongoc_server_description_destroy (sd);

   if (pooled) {
      mongoc_client_pool_push (pool, client);
      mongoc_client_pool_destroy (pool);
   } else {
      mongoc_client_destroy (client);
   }
}


static void
test_last_write_date_absent (void *ctx)
{
   _test_last_write_date_absent (false);
}


static void
test_last_write_date_absent_pooled (void *ctx)
{
   _test_last_write_date_absent (true);
}


static void
test_all_spec_tests (TestSuite *suite)
{
   char resolved[PATH_MAX];

   ASSERT (realpath (JSON_DIR "/max_staleness", resolved));
   install_json_test_suite (suite, resolved, &test_server_selection_logic_cb);
}

void
test_client_max_staleness_install (TestSuite *suite)
{
   test_all_spec_tests (suite);
   TestSuite_Add (
      suite, "/Client/max_staleness", test_mongoc_client_max_staleness);
   TestSuite_AddMockServerTest (suite,
                                "/Client/max_staleness/mongos",
                                test_mongos_max_staleness_read_pref);
   TestSuite_AddFull (suite,
                      "/Client/last_write_date",
                      test_last_write_date,
                      NULL,
                      NULL,
                      test_framework_skip_if_not_rs_version_5);
   TestSuite_AddFull (suite,
                      "/Client/last_write_date/pooled",
                      test_last_write_date_pooled,
                      NULL,
                      NULL,
                      test_framework_skip_if_not_rs_version_5);
   TestSuite_AddFull (suite,
                      "/Client/last_write_date_absent",
                      test_last_write_date_absent,
                      NULL,
                      NULL,
                      test_framework_skip_if_rs_version_5);
   TestSuite_AddFull (suite,
                      "/Client/last_write_date_absent/pooled",
                      test_last_write_date_absent_pooled,
                      NULL,
                      NULL,
                      test_framework_skip_if_rs_version_5);
}
