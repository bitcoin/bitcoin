#include <mongoc.h>

#include "mongoc-client-pool-private.h"

#include "TestSuite.h"
#include "test-conveniences.h"
#include "test-libmongoc.h"


static mongoc_stream_t *
cannot_resolve (const mongoc_uri_t *uri,
                const mongoc_host_list_t *host,
                void *user_data,
                bson_error_t *error)
{
   bson_set_error (error,
                   MONGOC_ERROR_STREAM,
                   MONGOC_ERROR_STREAM_NAME_RESOLUTION,
                   "Fake error for '%s'",
                   host->host);

   return NULL;
}


static void
server_selection_error_dns (const char *uri_str,
                            const char *errmsg,
                            bool expect_success,
                            bool pooled)
{
   mongoc_uri_t *uri;
   mongoc_client_pool_t *pool = NULL;
   mongoc_client_t *client;
   mongoc_collection_t *collection;
   bson_error_t error;
   bson_t *command;
   bson_t reply;
   bool success;

   uri = mongoc_uri_new (uri_str);
   ASSERT (uri);

   if (pooled && expect_success) {
      pool = mongoc_client_pool_new (uri);
      client = mongoc_client_pool_pop (pool);
   } else if (pooled) {
      /* we expect selection to fail; let the test finish faster */
      mongoc_uri_set_option_as_int32 (uri, "serverSelectionTimeoutMS", 2000);
      pool = mongoc_client_pool_new (uri);
      _mongoc_client_pool_set_stream_initiator (pool, cannot_resolve, NULL);
      client = mongoc_client_pool_pop (pool);
   } else {
      client = mongoc_client_new_from_uri (uri);
      if (!expect_success) {
         mongoc_client_set_stream_initiator (client, cannot_resolve, NULL);
      }
   }

   collection = mongoc_client_get_collection (client, "test", "test");

   command = tmp_bson ("{'ping': 1}");
   success = mongoc_collection_command_simple (
      collection, command, NULL, &reply, &error);
   ASSERT_OR_PRINT (success == expect_success, error);

   if (!success && errmsg) {
      ASSERT_CMPSTR (error.message, errmsg);
   }

   bson_destroy (&reply);
   mongoc_collection_destroy (collection);

   if (pooled) {
      mongoc_client_pool_push (pool, client);
      mongoc_client_pool_destroy (pool);
   } else {
      mongoc_client_destroy (client);
   }

   mongoc_uri_destroy (uri);
}

static void
test_server_selection_error_dns_direct_single (void)
{
   server_selection_error_dns (
      "mongodb://example-localhost:27017/",
      "No suitable servers found (`serverSelectionTryOnce` set): "
      "[Fake error for 'example-localhost']",
      false,
      false);
}

static void
test_server_selection_error_dns_direct_pooled (void *ctx)
{
   server_selection_error_dns (
      "mongodb://example-localhost:27017/",
      "No suitable servers found: `serverSelectionTimeoutMS` expired: "
      "[Fake error for 'example-localhost']",
      false,
      true);
}

static void
test_server_selection_error_dns_multi_fail_single (void)
{
   server_selection_error_dns (
      "mongodb://example-localhost:27017,other-example-localhost:27017/",
      "No suitable servers found (`serverSelectionTryOnce` set):"
      " [Fake error for 'example-localhost']"
      " [Fake error for 'other-example-localhost']",
      false,
      false);
}

static void
test_server_selection_error_dns_multi_fail_pooled (void *ctx)
{
   server_selection_error_dns (
      "mongodb://example-localhost:27017,other-example-localhost:27017/",
      "No suitable servers found: `serverSelectionTimeoutMS` expired:"
      " [Fake error for 'example-localhost']"
      " [Fake error for 'other-example-localhost']",
      false,
      true);
}

static void
_test_server_selection_error_dns_multi_success (bool pooled)
{
   char *uri_str;

   uri_str = bson_strdup_printf ("mongodb://example-localhost:27017,"
                                 "%s:%d,"
                                 "other-example-localhost:27017/",
                                 test_framework_get_host (),
                                 test_framework_get_port ());

   server_selection_error_dns (uri_str, "", true, pooled);

   bson_free (uri_str);
}

static void
test_server_selection_error_dns_multi_success_single (void *context)
{
   _test_server_selection_error_dns_multi_success (false);
}

static void
test_server_selection_error_dns_multi_success_pooled (void *context)
{
   _test_server_selection_error_dns_multi_success (true);
}

static void
_test_server_selection_uds_auth_failure (bool pooled)
{
   mongoc_uri_t *uri;
   mongoc_client_pool_t *pool = NULL;
   mongoc_client_t *client;
   bson_error_t error;
   bson_t reply;
   char *path;
   char *uri_str;

   path = test_framework_get_unix_domain_socket_path_escaped ();
   uri_str = bson_strdup_printf ("mongodb://user:wrongpass@%s", path);

   uri = mongoc_uri_new (uri_str);
   ASSERT (uri);

   if (pooled) {
      pool = mongoc_client_pool_new (uri);
#ifdef MONGOC_ENABLE_SSL
      test_framework_set_pool_ssl_opts (pool);
#endif
      client = mongoc_client_pool_pop (pool);
   } else {
      client = mongoc_client_new_from_uri (uri);
#ifdef MONGOC_ENABLE_SSL
      test_framework_set_ssl_opts (client);
#endif
   }

   capture_logs (true);

   ASSERT_OR_PRINT (
      !mongoc_client_get_server_status (client, NULL, &reply, &error), error);

   ASSERT_CMPINT (error.domain, ==, MONGOC_ERROR_CLIENT);
   ASSERT_CMPINT (error.code, ==, MONGOC_ERROR_CLIENT_AUTHENTICATE);

   if (pooled) {
      mongoc_client_pool_push (pool, client);
      mongoc_client_pool_destroy (pool);
   } else {
      mongoc_client_destroy (client);
   }

   bson_destroy (&reply);
   bson_free (path);
   bson_free (uri_str);
   mongoc_uri_destroy (uri);
}

static void
test_server_selection_uds_auth_failure_single (void *context)
{
   _test_server_selection_uds_auth_failure (false);
}

static void
test_server_selection_uds_auth_failure_pooled (void *context)
{
   _test_server_selection_uds_auth_failure (true);
}

static void
_test_server_selection_uds_not_found (bool pooled)
{
   mongoc_uri_t *uri;
   mongoc_client_pool_t *pool = NULL;
   mongoc_client_t *client;
   bson_error_t error;
   bson_t reply;

   uri = mongoc_uri_new ("mongodb:///tmp/mongodb-so-close.sock");
   ASSERT (uri);
   mongoc_uri_set_option_as_int32 (uri, "serverSelectionTimeoutMS", 100);

   if (pooled) {
      pool = mongoc_client_pool_new (uri);
#ifdef MONGOC_ENABLE_SSL
      test_framework_set_pool_ssl_opts (pool);
#endif
      client = mongoc_client_pool_pop (pool);
   } else {
      client = mongoc_client_new_from_uri (uri);
#ifdef MONGOC_ENABLE_SSL
      test_framework_set_ssl_opts (client);
#endif
   }

#ifdef MONGOC_ENABLE_SSL
   test_framework_set_ssl_opts (client);
#endif

   ASSERT (!mongoc_client_get_server_status (client, NULL, &reply, &error));
   ASSERT_CMPINT (error.domain, ==, MONGOC_ERROR_SERVER_SELECTION);
   ASSERT_CMPINT (error.code, ==, MONGOC_ERROR_SERVER_SELECTION_FAILURE);

   bson_destroy (&reply);

   if (pooled) {
      mongoc_client_pool_push (pool, client);
      mongoc_client_pool_destroy (pool);
   } else {
      mongoc_client_destroy (client);
   }

   mongoc_uri_destroy (uri);
}

static void
test_server_selection_uds_not_found_single (void *context)
{
   _test_server_selection_uds_not_found (false);
}

static void
test_server_selection_uds_not_found_pooled (void *context)
{
   _test_server_selection_uds_not_found (true);
}


void
test_server_selection_errors_install (TestSuite *suite)
{
   TestSuite_Add (suite,
                  "/server_selection/errors/dns/direct/single",
                  test_server_selection_error_dns_direct_single);
   TestSuite_AddFull (suite,
                      "/server_selection/errors/dns/direct/pooled",
                      test_server_selection_error_dns_direct_pooled,
                      NULL,
                      NULL,
                      test_framework_skip_if_slow);
   TestSuite_Add (suite,
                  "/server_selection/errors/dns/multi/fail/single",
                  test_server_selection_error_dns_multi_fail_single);
   TestSuite_AddFull (suite,
                      "/server_selection/errors/dns/multi/fail/pooled",
                      test_server_selection_error_dns_multi_fail_pooled,
                      NULL,
                      NULL,
                      test_framework_skip_if_slow);
   TestSuite_AddFull (suite,
                      "/server_selection/errors/dns/multi/success/single",
                      test_server_selection_error_dns_multi_success_single,
                      NULL,
                      NULL,
                      test_framework_skip_if_single);
   TestSuite_AddFull (suite,
                      "/server_selection/errors/dns/multi/success/pooled",
                      test_server_selection_error_dns_multi_success_pooled,
                      NULL,
                      NULL,
                      test_framework_skip_if_single);
   TestSuite_AddFull (suite,
                      "/server_selection/errors/uds/auth_failure/single",
                      test_server_selection_uds_auth_failure_single,
                      NULL,
                      NULL,
                      test_framework_skip_if_no_uds);
   TestSuite_AddFull (suite,
                      "/server_selection/errors/uds/auth_failure/pooled",
                      test_server_selection_uds_auth_failure_pooled,
                      NULL,
                      NULL,
                      test_framework_skip_if_no_uds);
   TestSuite_AddFull (suite,
                      "/server_selection/errors/uds/not_found/single",
                      test_server_selection_uds_not_found_single,
                      NULL,
                      NULL,
                      test_framework_skip_if_windows);
   TestSuite_AddFull (suite,
                      "/server_selection/errors/uds/not_found/pooled",
                      test_server_selection_uds_not_found_pooled,
                      NULL,
                      NULL,
                      test_framework_skip_if_windows);
}
