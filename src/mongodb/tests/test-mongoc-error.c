#include <mongoc.h>

#include "TestSuite.h"
#include "test-conveniences.h"
#include "test-libmongoc.h"
#include "mock_server/future.h"
#include "mock_server/future-functions.h"
#include "mock_server/mock-server.h"


#undef MONGOC_LOG_DOMAIN
#define MONGOC_LOG_DOMAIN "error-test"


static void
test_set_error_api_single (void)
{
   mongoc_client_t *client;
   int32_t unsupported_versions[] = {-1, 0, 3};
   int i;

   capture_logs (true);
   client = test_framework_client_new ();

   for (i = 0; i < sizeof (unsupported_versions) / sizeof (int32_t); i++) {
      ASSERT (!mongoc_client_set_error_api (client, unsupported_versions[i]));
      ASSERT_CAPTURED_LOG ("mongoc_client_set_error_api",
                           MONGOC_LOG_LEVEL_ERROR,
                           "Unsupported Error API Version");
   }

   mongoc_client_destroy (client);
}


static void
test_set_error_api_pooled (void)
{
   mongoc_client_pool_t *pool;
   mongoc_client_t *client;
   int32_t unsupported_versions[] = {-1, 0, 3};
   int i;

   capture_logs (true);
   pool = test_framework_client_pool_new ();

   for (i = 0; i < sizeof (unsupported_versions) / sizeof (int32_t); i++) {
      ASSERT (
         !mongoc_client_pool_set_error_api (pool, unsupported_versions[i]));
      ASSERT_CAPTURED_LOG ("mongoc_client_pool_set_error_api",
                           MONGOC_LOG_LEVEL_ERROR,
                           "Unsupported Error API Version");
   }

   client = mongoc_client_pool_pop (pool);
   ASSERT (!mongoc_client_set_error_api (client, 1));
   ASSERT_CAPTURED_LOG ("mongoc_client_set_error_api",
                        MONGOC_LOG_LEVEL_ERROR,
                        "Cannot set Error API Version on a pooled client");

   mongoc_client_pool_push (pool, client);
   mongoc_client_pool_destroy (pool);
}


static void
_test_command_error (int32_t error_api_version)
{
   mock_server_t *server;
   mongoc_client_t *client;
   bson_t reply;
   bson_error_t error;
   future_t *future;
   request_t *request;

   server = mock_server_with_autoismaster (0);
   mock_server_run (server);
   client = mongoc_client_new_from_uri (mock_server_get_uri (server));

   if (error_api_version != 0) {
      BSON_ASSERT (mongoc_client_set_error_api (client, error_api_version));
   }

   future = future_client_command_simple (
      client, "db", tmp_bson ("{'foo': 1}"), NULL, &reply, &error);
   request =
      mock_server_receives_command (server, "db", MONGOC_QUERY_SLAVE_OK, NULL);
   mock_server_replies_simple (request,
                               "{'ok': 0, 'code': 42, 'errmsg': 'foo'}");
   ASSERT (!future_get_bool (future));

   if (error_api_version >= 2) {
      ASSERT_ERROR_CONTAINS (error, MONGOC_ERROR_SERVER, 42, "foo");
   } else {
      ASSERT_ERROR_CONTAINS (error, MONGOC_ERROR_QUERY, 42, "foo");
   }

   future_destroy (future);
   request_destroy (request);
   bson_destroy (&reply);
   mongoc_client_destroy (client);
   mock_server_destroy (server);
}


static void
test_command_error_default (void)
{
   _test_command_error (0);
}


static void
test_command_error_v1 (void)
{
   _test_command_error (1);
}


static void
test_command_error_v2 (void)
{
   _test_command_error (2);
}


void
test_error_install (TestSuite *suite)
{
   TestSuite_AddLive (
      suite, "/Error/set_api/single", test_set_error_api_single);
   TestSuite_AddLive (
      suite, "/Error/set_api/pooled", test_set_error_api_pooled);
   TestSuite_AddMockServerTest (
      suite, "/Error/command/default", test_command_error_default);
   TestSuite_AddMockServerTest (
      suite, "/Error/command/v1", test_command_error_v1);
   TestSuite_AddMockServerTest (
      suite, "/Error/command/v2", test_command_error_v2);
}
