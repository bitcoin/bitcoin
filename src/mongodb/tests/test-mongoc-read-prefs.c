#include <mongoc.h>
#include <mongoc-uri-private.h>

#include "TestSuite.h"
#include "mock_server/future.h"
#include "mock_server/future-functions.h"
#include "mock_server/mock-server.h"
#include "test-conveniences.h"


static bool
_can_be_command (const char *query)
{
   return (!bson_empty (tmp_bson (query)));
}

static void
_test_op_query (const mongoc_uri_t *uri,
                mock_server_t *server,
                const char *query_in,
                mongoc_read_prefs_t *read_prefs,
                mongoc_query_flags_t expected_query_flags,
                const char *expected_query)
{
   mongoc_client_t *client;
   mongoc_collection_t *collection;
   mongoc_cursor_t *cursor;
   const bson_t *doc;
   bson_t b = BSON_INITIALIZER;
   future_t *future;
   request_t *request;

   client = mongoc_client_new_from_uri (uri);
   collection = mongoc_client_get_collection (client, "test", "test");
   mongoc_collection_set_read_prefs (collection, read_prefs);

   cursor = mongoc_collection_find (collection,
                                    MONGOC_QUERY_NONE,
                                    0,
                                    1,
                                    0,
                                    tmp_bson (query_in),
                                    NULL,
                                    read_prefs);

   future = future_cursor_next (cursor, &doc);

   request = mock_server_receives_query (
      server, "test.test", expected_query_flags, 0, 1, expected_query, NULL);

   mock_server_replies (request,
                        MONGOC_REPLY_NONE, /* flags */
                        0,                 /* cursorId */
                        0,                 /* startingFrom */
                        1,                 /* numberReturned */
                        "{'a': 1}");

   /* mongoc_cursor_next returned true */
   BSON_ASSERT (future_get_bool (future));

   request_destroy (request);
   future_destroy (future);
   mongoc_cursor_destroy (cursor);
   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
   bson_destroy (&b);
}


static void
_test_find_command (const mongoc_uri_t *uri,
                    mock_server_t *server,
                    const char *query_in,
                    mongoc_read_prefs_t *read_prefs,
                    mongoc_query_flags_t expected_find_cmd_query_flags,
                    const char *expected_find_cmd)
{
   mongoc_client_t *client;
   mongoc_collection_t *collection;
   mongoc_cursor_t *cursor;
   const bson_t *doc;
   bson_t b = BSON_INITIALIZER;
   future_t *future;
   request_t *request;

   client = mongoc_client_new_from_uri (uri);
   collection = mongoc_client_get_collection (client, "test", "test");
   mongoc_collection_set_read_prefs (collection, read_prefs);

   cursor = mongoc_collection_find (collection,
                                    MONGOC_QUERY_NONE,
                                    0,
                                    1,
                                    0,
                                    tmp_bson (query_in),
                                    NULL,
                                    read_prefs);

   future = future_cursor_next (cursor, &doc);

   request = mock_server_receives_command (
      server, "test", expected_find_cmd_query_flags, expected_find_cmd);

   mock_server_replies (request,
                        MONGOC_REPLY_NONE, /* flags */
                        0,                 /* cursorId */
                        0,                 /* startingFrom */
                        1,                 /* numberReturned */
                        "{'ok': 1,"
                        " 'cursor': {"
                        "    'id': 0,"
                        "    'ns': 'db.collection',"
                        "    'firstBatch': [{'a': 1}]}}");

   /* mongoc_cursor_next returned true */
   BSON_ASSERT (future_get_bool (future));

   request_destroy (request);
   future_destroy (future);
   mongoc_cursor_destroy (cursor);
   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
   bson_destroy (&b);
}

static void
_test_command (const mongoc_uri_t *uri,
               mock_server_t *server,
               const char *command,
               mongoc_read_prefs_t *read_prefs,
               mongoc_query_flags_t expected_query_flags,
               const char *expected_query)
{
   mongoc_client_t *client;
   mongoc_collection_t *collection;
   mongoc_cursor_t *cursor;
   const bson_t *doc;
   bson_t b = BSON_INITIALIZER;
   future_t *future;
   request_t *request;

   client = mongoc_client_new_from_uri (uri);
   collection = mongoc_client_get_collection (client, "test", "test");
   mongoc_collection_set_read_prefs (collection, read_prefs);

   cursor = mongoc_collection_command (collection,
                                       MONGOC_QUERY_NONE,
                                       0,
                                       1,
                                       0,
                                       tmp_bson (command),
                                       NULL,
                                       read_prefs);

   future = future_cursor_next (cursor, &doc);

   request = mock_server_receives_command (
      server, "test", expected_query_flags, expected_query);

   mock_server_replies (request,
                        MONGOC_REPLY_NONE, /* flags */
                        0,                 /* cursorId */
                        0,                 /* startingFrom */
                        1,                 /* numberReturned */
                        "{'ok': 1}");

   /* mongoc_cursor_next returned true */
   BSON_ASSERT (future_get_bool (future));

   request_destroy (request);
   future_destroy (future);
   mongoc_cursor_destroy (cursor);
   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
   bson_destroy (&b);
}

static void
_test_command_simple (const mongoc_uri_t *uri,
                      mock_server_t *server,
                      const char *command,
                      mongoc_read_prefs_t *read_prefs,
                      mongoc_query_flags_t expected_query_flags,
                      const char *expected_query)
{
   mongoc_client_t *client;
   mongoc_collection_t *collection;
   future_t *future;
   request_t *request;

   client = mongoc_client_new_from_uri (uri);
   collection = mongoc_client_get_collection (client, "test", "test");
   mongoc_collection_set_read_prefs (collection, read_prefs);

   future = future_client_command_simple (
      client, "test", tmp_bson (command), read_prefs, NULL, NULL);

   request = mock_server_receives_command (
      server, "test", expected_query_flags, expected_query);

   mock_server_replies (request,
                        MONGOC_REPLY_NONE, /* flags */
                        0,                 /* cursorId */
                        0,                 /* startingFrom */
                        1,                 /* numberReturned */
                        "{'ok': 1}");

   /* mongoc_cursor_next returned true */
   BSON_ASSERT (future_get_bool (future));

   request_destroy (request);
   future_destroy (future);
   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
}


typedef enum {
   READ_PREF_TEST_STANDALONE,
   READ_PREF_TEST_MONGOS,
   READ_PREF_TEST_PRIMARY,
   READ_PREF_TEST_SECONDARY,
} read_pref_test_type_t;


static mock_server_t *
_run_server (read_pref_test_type_t test_type, int32_t max_wire_version)
{
   mock_server_t *server;

   server = mock_server_new ();
   mock_server_run (server);

   switch (test_type) {
   case READ_PREF_TEST_STANDALONE:
      mock_server_auto_ismaster (server,
                                 "{'ok': 1,"
                                 " 'maxWireVersion': %d,"
                                 " 'ismaster': true}",
                                 max_wire_version);
      break;
   case READ_PREF_TEST_MONGOS:
      mock_server_auto_ismaster (server,
                                 "{'ok': 1,"
                                 " 'maxWireVersion': %d,"
                                 " 'ismaster': true,"
                                 " 'msg': 'isdbgrid'}",
                                 max_wire_version);
      break;
   case READ_PREF_TEST_PRIMARY:
      mock_server_auto_ismaster (server,
                                 "{'ok': 1,"
                                 " 'maxWireVersion': %d,"
                                 " 'ismaster': true,"
                                 " 'setName': 'rs',"
                                 " 'hosts': ['%s']}",
                                 max_wire_version,
                                 mock_server_get_host_and_port (server));
      break;
   case READ_PREF_TEST_SECONDARY:
      mock_server_auto_ismaster (server,
                                 "{'ok': 1,"
                                 " 'maxWireVersion': %d,"
                                 " 'ismaster': false,"
                                 " 'secondary': true,"
                                 " 'setName': 'rs',"
                                 " 'hosts': ['%s']}",
                                 max_wire_version,
                                 mock_server_get_host_and_port (server));
      break;
   default:
      fprintf (stderr, "Invalid test_type: : %d\n", test_type);
      abort ();
   }

   return server;
}


static mongoc_uri_t *
_get_uri (mock_server_t *server, read_pref_test_type_t test_type)
{
   mongoc_uri_t *uri;

   uri = mongoc_uri_copy (mock_server_get_uri (server));

   switch (test_type) {
   case READ_PREF_TEST_PRIMARY:
   case READ_PREF_TEST_SECONDARY:
      mongoc_uri_set_option_as_utf8 (uri, "replicaSet", "rs");
      break;
   case READ_PREF_TEST_STANDALONE:
   case READ_PREF_TEST_MONGOS:
   default:
      break;
   }

   return uri;
}


static void
_test_read_prefs (read_pref_test_type_t test_type,
                  mongoc_read_prefs_t *read_prefs,
                  const char *query_in,
                  const char *expected_query,
                  mongoc_query_flags_t expected_query_flags,
                  const char *expected_find_cmd,
                  mongoc_query_flags_t expected_find_cmd_query_flags)
{
   mock_server_t *server;
   mongoc_uri_t *uri;

   server = _run_server (test_type, 3);
   uri = _get_uri (server, test_type);

   _test_op_query (
      uri, server, query_in, read_prefs, expected_query_flags, expected_query);

   if (_can_be_command (query_in)) {
      _test_command (uri,
                     server,
                     query_in,
                     read_prefs,
                     expected_query_flags,
                     expected_query);

      _test_command_simple (uri,
                            server,
                            query_in,
                            read_prefs,
                            expected_query_flags,
                            expected_query);
   }

   mock_server_destroy (server);
   mongoc_uri_destroy (uri);

   server = _run_server (test_type, 4);
   uri = _get_uri (server, test_type);

   _test_find_command (uri,
                       server,
                       query_in,
                       read_prefs,
                       expected_find_cmd_query_flags,
                       expected_find_cmd);

   mock_server_destroy (server);
   mongoc_uri_destroy (uri);
}


/* test that a NULL read pref is the same as PRIMARY */
static void
test_read_prefs_standalone_null (void)
{
   _test_read_prefs (READ_PREF_TEST_STANDALONE,
                     NULL,
                     "{}",
                     "{}",
                     MONGOC_QUERY_SLAVE_OK,
                     "{'find': 'test', 'filter':  {}}",
                     MONGOC_QUERY_SLAVE_OK);

   _test_read_prefs (READ_PREF_TEST_STANDALONE,
                     NULL,
                     "{'a': 1}",
                     "{'a': 1}",
                     MONGOC_QUERY_SLAVE_OK,
                     "{'find': 'test', 'filter':  {'a': 1}}",
                     MONGOC_QUERY_SLAVE_OK);
}

static void
test_read_prefs_standalone_primary (void)
{
   mongoc_read_prefs_t *read_prefs;

   /* Server Selection Spec: for topology type single and server types other
    * than mongos, "clients MUST always set the slaveOK wire protocol flag on
    * reads to ensure that any server type can handle the request."
    * */
   read_prefs = mongoc_read_prefs_new (MONGOC_READ_PRIMARY);

   _test_read_prefs (READ_PREF_TEST_STANDALONE,
                     read_prefs,
                     "{}",
                     "{}",
                     MONGOC_QUERY_SLAVE_OK,
                     "{'find': 'test', 'filter':  {}}",
                     MONGOC_QUERY_SLAVE_OK);

   _test_read_prefs (READ_PREF_TEST_STANDALONE,
                     read_prefs,
                     "{'a': 1}",
                     "{'a': 1}",
                     MONGOC_QUERY_SLAVE_OK,
                     "{'find': 'test', 'filter':  {'a': 1}}",
                     MONGOC_QUERY_SLAVE_OK);

   mongoc_read_prefs_destroy (read_prefs);
}


static void
test_read_prefs_standalone_secondary (void)
{
   mongoc_read_prefs_t *read_prefs;

   read_prefs = mongoc_read_prefs_new (MONGOC_READ_SECONDARY);

   _test_read_prefs (READ_PREF_TEST_STANDALONE,
                     read_prefs,
                     "{}",
                     "{}",
                     MONGOC_QUERY_SLAVE_OK,
                     "{'find': 'test', 'filter':  {}}",
                     MONGOC_QUERY_SLAVE_OK);

   _test_read_prefs (READ_PREF_TEST_STANDALONE,
                     read_prefs,
                     "{'a': 1}",
                     "{'a': 1}",
                     MONGOC_QUERY_SLAVE_OK,
                     "{'find': 'test', 'filter':  {'a': 1}}",
                     MONGOC_QUERY_SLAVE_OK);

   mongoc_read_prefs_destroy (read_prefs);
}


static void
test_read_prefs_standalone_tags (void)
{
   bson_t b = BSON_INITIALIZER;
   mongoc_read_prefs_t *read_prefs;

   bson_append_utf8 (&b, "dc", 2, "ny", 2);

   read_prefs = mongoc_read_prefs_new (MONGOC_READ_SECONDARY_PREFERRED);
   mongoc_read_prefs_add_tag (read_prefs, &b);
   mongoc_read_prefs_add_tag (read_prefs, NULL);

   _test_read_prefs (READ_PREF_TEST_STANDALONE,
                     read_prefs,
                     "{}",
                     "{}",
                     MONGOC_QUERY_SLAVE_OK,
                     "{'find': 'test', 'filter':  {}}",
                     MONGOC_QUERY_SLAVE_OK);

   _test_read_prefs (READ_PREF_TEST_STANDALONE,
                     read_prefs,
                     "{'a': 1}",
                     "{'a': 1}",
                     MONGOC_QUERY_SLAVE_OK,
                     "{'find': 'test', 'filter':  {'a': 1}}",
                     MONGOC_QUERY_SLAVE_OK);

   mongoc_read_prefs_destroy (read_prefs);
}


static void
test_read_prefs_primary_rsprimary (void)
{
   mongoc_read_prefs_t *read_prefs;

   read_prefs = mongoc_read_prefs_new (MONGOC_READ_PRIMARY);

   _test_read_prefs (READ_PREF_TEST_PRIMARY,
                     read_prefs,
                     "{}",
                     "{}",
                     MONGOC_QUERY_NONE,
                     "{'find': 'test', 'filter':  {}}",
                     MONGOC_QUERY_NONE);

   _test_read_prefs (READ_PREF_TEST_PRIMARY,
                     read_prefs,
                     "{'a': 1}",
                     "{'a': 1}",
                     MONGOC_QUERY_NONE,
                     "{'find': 'test', 'filter':  {'a': 1}}",
                     MONGOC_QUERY_NONE);

   mongoc_read_prefs_destroy (read_prefs);
}


static void
test_read_prefs_secondary_rssecondary (void)
{
   mongoc_read_prefs_t *read_prefs;

   read_prefs = mongoc_read_prefs_new (MONGOC_READ_SECONDARY);

   _test_read_prefs (READ_PREF_TEST_SECONDARY,
                     read_prefs,
                     "{}",
                     "{}",
                     MONGOC_QUERY_SLAVE_OK,
                     "{'find': 'test', 'filter':  {}}",
                     MONGOC_QUERY_SLAVE_OK);

   _test_read_prefs (READ_PREF_TEST_SECONDARY,
                     read_prefs,
                     "{'a': 1}",
                     "{'a': 1}",
                     MONGOC_QUERY_SLAVE_OK,
                     "{'find': 'test', 'filter':  {'a': 1}}",
                     MONGOC_QUERY_SLAVE_OK);

   mongoc_read_prefs_destroy (read_prefs);
}


/* test that a NULL read pref is the same as PRIMARY */
static void
test_read_prefs_mongos_null (void)
{
   _test_read_prefs (READ_PREF_TEST_MONGOS,
                     NULL,
                     "{}",
                     "{}",
                     MONGOC_QUERY_NONE,
                     "{'find': 'test', 'filter':  {}}",
                     MONGOC_QUERY_NONE);

   _test_read_prefs (READ_PREF_TEST_MONGOS,
                     NULL,
                     "{'a': 1}",
                     "{'a': 1}",
                     MONGOC_QUERY_NONE,
                     "{'find': 'test', 'filter':  {}}",
                     MONGOC_QUERY_NONE);
}


static void
test_read_prefs_mongos_primary (void)
{
   mongoc_read_prefs_t *read_prefs;

   read_prefs = mongoc_read_prefs_new (MONGOC_READ_PRIMARY);

   _test_read_prefs (READ_PREF_TEST_MONGOS,
                     read_prefs,
                     "{}",
                     "{}",
                     MONGOC_QUERY_NONE,
                     "{'find': 'test', 'filter':  {}}",
                     MONGOC_QUERY_NONE);

   _test_read_prefs (READ_PREF_TEST_MONGOS,
                     read_prefs,
                     "{'a': 1}",
                     "{'a': 1}",
                     MONGOC_QUERY_NONE,
                     "{'find': 'test', 'filter':  {'a': 1}}",
                     MONGOC_QUERY_NONE);

   mongoc_read_prefs_destroy (read_prefs);
}


static void
test_read_prefs_mongos_secondary (void)
{
   mongoc_read_prefs_t *read_prefs;

   read_prefs = mongoc_read_prefs_new (MONGOC_READ_SECONDARY);

   _test_read_prefs (READ_PREF_TEST_MONGOS,
                     read_prefs,
                     "{}",
                     "{'$query': {}, '$readPreference': {'mode': 'secondary'}}",
                     MONGOC_QUERY_SLAVE_OK,
                     "{'$query': {'find': 'test', 'filter':  {}},"
                     " '$readPreference': {'mode': 'secondary'}}",
                     MONGOC_QUERY_SLAVE_OK);

   _test_read_prefs (
      READ_PREF_TEST_MONGOS,
      read_prefs,
      "{'a': 1}",
      "{'$query': {'a': 1}, '$readPreference': {'mode': 'secondary'}}",
      MONGOC_QUERY_SLAVE_OK,
      "{'$query': {'find': 'test', 'filter':  {'a': 1}},"
      " '$readPreference': {'mode': 'secondary'}}",
      MONGOC_QUERY_SLAVE_OK);

   _test_read_prefs (
      READ_PREF_TEST_MONGOS,
      read_prefs,
      "{'$query': {'a': 1}}",
      "{'$query': {'a': 1}, '$readPreference': {'mode': 'secondary'}}",
      MONGOC_QUERY_SLAVE_OK,
      "{'$query': {'find': 'test', 'filter':  {'a': 1}},"
      " '$readPreference': {'mode': 'secondary'}}",
      MONGOC_QUERY_SLAVE_OK);

   mongoc_read_prefs_destroy (read_prefs);
}


static void
test_read_prefs_mongos_secondary_preferred (void)
{
   mongoc_read_prefs_t *read_prefs;

   read_prefs = mongoc_read_prefs_new (MONGOC_READ_SECONDARY_PREFERRED);

   /* $readPreference not sent, only slaveOk */
   _test_read_prefs (READ_PREF_TEST_MONGOS,
                     read_prefs,
                     "{}",
                     "{}",
                     MONGOC_QUERY_SLAVE_OK,
                     "{'find': 'test', 'filter':  {}}",
                     MONGOC_QUERY_SLAVE_OK);

   _test_read_prefs (READ_PREF_TEST_MONGOS,
                     read_prefs,
                     "{'a': 1}",
                     "{'a': 1}",
                     MONGOC_QUERY_SLAVE_OK,
                     "{'find': 'test', 'filter':  {'a': 1}}",
                     MONGOC_QUERY_SLAVE_OK);

   mongoc_read_prefs_destroy (read_prefs);
}


static void
test_read_prefs_mongos_tags (void)
{
   bson_t b = BSON_INITIALIZER;
   mongoc_read_prefs_t *read_prefs;

   bson_append_utf8 (&b, "dc", 2, "ny", 2);

   read_prefs = mongoc_read_prefs_new (MONGOC_READ_SECONDARY_PREFERRED);
   mongoc_read_prefs_add_tag (read_prefs, &b);
   mongoc_read_prefs_add_tag (read_prefs, NULL);

   _test_read_prefs (
      READ_PREF_TEST_MONGOS,
      read_prefs,
      "{}",
      "{'$query': {}, '$readPreference': {'mode': 'secondaryPreferred',"
      "                                   'tags': [{'dc': 'ny'}, {}]}}",
      MONGOC_QUERY_SLAVE_OK,
      "{'$query': {'find': 'test', 'filter':  {}},"
      " '$readPreference': {'mode': 'secondaryPreferred',"
      "                             'tags': [{'dc': 'ny'}, {}]}}",
      MONGOC_QUERY_SLAVE_OK);

   _test_read_prefs (
      READ_PREF_TEST_MONGOS,
      read_prefs,
      "{'a': 1}",
      "{'$query': {'a': 1},"
      " '$readPreference': {'mode': 'secondaryPreferred',"
      "                     'tags': [{'dc': 'ny'}, {}]}}",
      MONGOC_QUERY_SLAVE_OK,
      "{'$query': {'find': 'test', 'filter':  {}},"
      " '$readPreference': {'mode': 'secondaryPreferred',"
      "                             'tags': [{'dc': 'ny'}, {}]}}",
      MONGOC_QUERY_SLAVE_OK);

   mongoc_read_prefs_destroy (read_prefs);
}


void
test_read_prefs_install (TestSuite *suite)
{
   TestSuite_AddMockServerTest (
      suite, "/ReadPrefs/standalone/null", test_read_prefs_standalone_null);
   TestSuite_AddMockServerTest (suite,
                                "/ReadPrefs/standalone/primary",
                                test_read_prefs_standalone_primary);
   TestSuite_AddMockServerTest (suite,
                                "/ReadPrefs/standalone/secondary",
                                test_read_prefs_standalone_secondary);
   TestSuite_AddMockServerTest (
      suite, "/ReadPrefs/standalone/tags", test_read_prefs_standalone_tags);
   TestSuite_AddMockServerTest (
      suite, "/ReadPrefs/rsprimary/primary", test_read_prefs_primary_rsprimary);
   TestSuite_AddMockServerTest (suite,
                                "/ReadPrefs/rssecondary/secondary",
                                test_read_prefs_secondary_rssecondary);
   TestSuite_AddMockServerTest (
      suite, "/ReadPrefs/mongos/null", test_read_prefs_mongos_null);
   TestSuite_AddMockServerTest (
      suite, "/ReadPrefs/mongos/primary", test_read_prefs_mongos_primary);
   TestSuite_AddMockServerTest (
      suite, "/ReadPrefs/mongos/secondary", test_read_prefs_mongos_secondary);
   TestSuite_AddMockServerTest (suite,
                                "/ReadPrefs/mongos/secondaryPreferred",
                                test_read_prefs_mongos_secondary_preferred);
   TestSuite_AddMockServerTest (
      suite, "/ReadPrefs/mongos/tags", test_read_prefs_mongos_tags);
}
