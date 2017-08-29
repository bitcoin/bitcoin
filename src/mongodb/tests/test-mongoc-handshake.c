/*
 * Copyright 2016 MongoDB, Inc.
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

#include <mongoc.h>
#ifdef _POSIX_VERSION
#include <sys/utsname.h>
#endif

#include "mongoc-client-private.h"
#include "mongoc-handshake.h"
#include "mongoc-handshake-private.h"

#include "TestSuite.h"
#include "test-libmongoc.h"
#include "test-conveniences.h"
#include "mock_server/future.h"
#include "mock_server/future-functions.h"
#include "mock_server/mock-server.h"

/*
 * Call this before any test which uses mongoc_handshake_data_append, to
 * reset the global state and unfreeze the handshake struct. Call it
 * after a test so later tests don't have a weird handshake document
 *
 * This is not safe to call while we have any clients or client pools running!
 */
static void
_reset_handshake (void)
{
   _mongoc_handshake_cleanup ();
   _mongoc_handshake_init ();
}

static void
test_mongoc_handshake_appname_in_uri (void)
{
   char long_string[MONGOC_HANDSHAKE_APPNAME_MAX + 2];
   char *uri_str;
   const char *good_uri = "mongodb://host/?" MONGOC_URI_APPNAME "=mongodump";
   mongoc_uri_t *uri;
   const char *appname = "mongodump";
   const char *value;

   memset (long_string, 'a', MONGOC_HANDSHAKE_APPNAME_MAX + 1);
   long_string[MONGOC_HANDSHAKE_APPNAME_MAX + 1] = '\0';

   /* Shouldn't be able to set with appname really long */
   capture_logs (true);
   uri_str = bson_strdup_printf ("mongodb://a/?" MONGOC_URI_APPNAME "=%s",
                                 long_string);
   ASSERT (!mongoc_client_new (uri_str));
   ASSERT_CAPTURED_LOG ("_mongoc_topology_scanner_set_appname",
                        MONGOC_LOG_LEVEL_WARNING,
                        "Unsupported value");
   capture_logs (false);

   uri = mongoc_uri_new (good_uri);
   ASSERT (uri);
   value = mongoc_uri_get_appname (uri);
   ASSERT (value);
   ASSERT_CMPSTR (appname, value);
   mongoc_uri_destroy (uri);

   uri = mongoc_uri_new (NULL);
   ASSERT (uri);
   ASSERT (!mongoc_uri_set_appname (uri, long_string));
   ASSERT (mongoc_uri_set_appname (uri, appname));
   value = mongoc_uri_get_appname (uri);
   ASSERT (value);
   ASSERT_CMPSTR (appname, value);
   mongoc_uri_destroy (uri);

   bson_free (uri_str);
}

static void
test_mongoc_handshake_appname_frozen_single (void)
{
   mongoc_client_t *client;
   const char *good_uri = "mongodb://host/?" MONGOC_URI_APPNAME "=mongodump";

   client = mongoc_client_new (good_uri);

   /* Shouldn't be able to set appname again */
   capture_logs (true);
   ASSERT (!mongoc_client_set_appname (client, "a"));
   ASSERT_CAPTURED_LOG ("_mongoc_topology_scanner_set_appname",
                        MONGOC_LOG_LEVEL_ERROR,
                        "Cannot set appname more than once");
   capture_logs (false);

   mongoc_client_destroy (client);
}

static void
test_mongoc_handshake_appname_frozen_pooled (void)
{
   mongoc_client_pool_t *pool;
   const char *good_uri = "mongodb://host/?" MONGOC_URI_APPNAME "=mongodump";
   mongoc_uri_t *uri;

   uri = mongoc_uri_new (good_uri);

   pool = mongoc_client_pool_new (uri);
   capture_logs (true);
   ASSERT (!mongoc_client_pool_set_appname (pool, "test"));
   ASSERT_CAPTURED_LOG ("_mongoc_topology_scanner_set_appname",
                        MONGOC_LOG_LEVEL_ERROR,
                        "Cannot set appname more than once");
   capture_logs (false);

   mongoc_client_pool_destroy (pool);
   mongoc_uri_destroy (uri);
}

static void
_check_arch_string_valid (const char *arch)
{
#ifdef _POSIX_VERSION
   struct utsname system_info;

   ASSERT (uname (&system_info) >= 0);
   ASSERT_CMPSTR (system_info.machine, arch);
#endif
   ASSERT (strlen (arch) > 0);
}

static void
_check_os_version_valid (const char *os_version)
{
#if defined(__linux__) || defined(_WIN32)
   /* On linux we search the filesystem for os version or use uname.
    * On windows we call GetSystemInfo(). */
   ASSERT (os_version);
   ASSERT (strlen (os_version) > 0);
#elif defined(_POSIX_VERSION)
   /* On a non linux posix systems, we just call uname() */
   struct utsname system_info;

   ASSERT (uname (&system_info) >= 0);
   ASSERT (os_version);
   ASSERT_CMPSTR (system_info.release, os_version);
#endif
}

static void
test_mongoc_handshake_data_append_success (void)
{
   mock_server_t *server;
   mongoc_uri_t *uri;
   mongoc_client_t *client;
   mongoc_client_pool_t *pool;
   request_t *request;
   const bson_t *request_doc;
   bson_iter_t iter;
   bson_iter_t md_iter;
   bson_iter_t inner_iter;
   const char *val;

   const char *driver_name = "php driver";
   const char *driver_version = "version abc";
   const char *platform = "./configure -nottoomanyflags";

   char big_string[HANDSHAKE_MAX_SIZE];

   memset (big_string, 'a', HANDSHAKE_MAX_SIZE - 1);
   big_string[HANDSHAKE_MAX_SIZE - 1] = '\0';

   _reset_handshake ();
   /* Make sure setting the handshake works */
   ASSERT (
      mongoc_handshake_data_append (driver_name, driver_version, platform));

   server = mock_server_new ();
   mock_server_run (server);
   uri = mongoc_uri_copy (mock_server_get_uri (server));
   mongoc_uri_set_option_as_int32 (uri, MONGOC_URI_HEARTBEATFREQUENCYMS, 500);
   mongoc_uri_set_option_as_utf8 (uri, MONGOC_URI_APPNAME, "testapp");
   pool = mongoc_client_pool_new (uri);

   /* Force topology scanner to start */
   client = mongoc_client_pool_pop (pool);

   request = mock_server_receives_ismaster (server);
   ASSERT (request);
   request_doc = request_get_doc (request, 0);
   ASSERT (request_doc);
   ASSERT (bson_has_field (request_doc, "isMaster"));
   ASSERT (bson_has_field (request_doc, HANDSHAKE_FIELD));

   ASSERT (bson_iter_init_find (&iter, request_doc, HANDSHAKE_FIELD));
   ASSERT (bson_iter_recurse (&iter, &md_iter));

   ASSERT (bson_iter_find (&md_iter, "application"));
   ASSERT (BSON_ITER_HOLDS_DOCUMENT (&md_iter));
   ASSERT (bson_iter_recurse (&md_iter, &inner_iter));
   ASSERT (bson_iter_find (&inner_iter, "name"));
   val = bson_iter_utf8 (&inner_iter, NULL);
   ASSERT (val);
   ASSERT_CMPSTR (val, "testapp");

   /* Make sure driver.name and driver.version and platform are all right */
   ASSERT (bson_iter_find (&md_iter, "driver"));
   ASSERT (BSON_ITER_HOLDS_DOCUMENT (&md_iter));
   ASSERT (bson_iter_recurse (&md_iter, &inner_iter));
   ASSERT (bson_iter_find (&inner_iter, "name"));
   ASSERT (BSON_ITER_HOLDS_UTF8 (&inner_iter));
   val = bson_iter_utf8 (&inner_iter, NULL);
   ASSERT (val);
   ASSERT (strstr (val, driver_name) != NULL);

   ASSERT (bson_iter_find (&inner_iter, "version"));
   ASSERT (BSON_ITER_HOLDS_UTF8 (&inner_iter));
   val = bson_iter_utf8 (&inner_iter, NULL);
   ASSERT (val);
   ASSERT (strstr (val, driver_version));

   /* Check os type not empty */
   ASSERT (bson_iter_find (&md_iter, "os"));
   ASSERT (BSON_ITER_HOLDS_DOCUMENT (&md_iter));
   ASSERT (bson_iter_recurse (&md_iter, &inner_iter));

   ASSERT (bson_iter_find (&inner_iter, "type"));
   ASSERT (BSON_ITER_HOLDS_UTF8 (&inner_iter));
   val = bson_iter_utf8 (&inner_iter, NULL);
   ASSERT (val);
   ASSERT (strlen (val) > 0);

   /* Check os version valid */
   ASSERT (bson_iter_find (&inner_iter, "version"));
   ASSERT (BSON_ITER_HOLDS_UTF8 (&inner_iter));
   val = bson_iter_utf8 (&inner_iter, NULL);
   _check_os_version_valid (val);

   /* Check os arch is valid */
   ASSERT (bson_iter_find (&inner_iter, "architecture"));
   ASSERT (BSON_ITER_HOLDS_UTF8 (&inner_iter));
   val = bson_iter_utf8 (&inner_iter, NULL);
   ASSERT (val);
   _check_arch_string_valid (val);

   /* Not checking os_name, as the spec says it can be NULL. */

   /* Check platform field ok */
   ASSERT (bson_iter_find (&md_iter, "platform"));
   ASSERT (BSON_ITER_HOLDS_UTF8 (&md_iter));
   val = bson_iter_utf8 (&md_iter, NULL);
   ASSERT (val);
   if (strlen (val) <
       250) { /* standard val are < 100, may be truncated on some platform */
      ASSERT (strstr (val, platform) != NULL);
   }

   mock_server_replies_simple (request, "{'ok': 1, 'ismaster': true}");
   request_destroy (request);

   /* Cleanup */
   mongoc_client_pool_push (pool, client);
   mongoc_client_pool_destroy (pool);
   mongoc_uri_destroy (uri);
   mock_server_destroy (server);

   _reset_handshake ();
}

static void
test_mongoc_handshake_data_append_after_cmd (void)
{
   mongoc_client_pool_t *pool;
   mongoc_client_t *client;
   mongoc_uri_t *uri;

   _reset_handshake ();

   uri = mongoc_uri_new ("mongodb://127.0.0.1/?" MONGOC_URI_MAXPOOLSIZE
                         "=1&" MONGOC_URI_MINPOOLSIZE "=1");

   /* Make sure that after we pop a client we can't set global handshake */
   pool = mongoc_client_pool_new (uri);

   client = mongoc_client_pool_pop (pool);

   capture_logs (true);
   ASSERT (!mongoc_handshake_data_append ("a", "a", "a"));
   ASSERT_CAPTURED_LOG ("mongoc_handshake_data_append",
                        MONGOC_LOG_LEVEL_ERROR,
                        "Cannot set handshake more than once");
   capture_logs (false);

   mongoc_client_pool_push (pool, client);

   mongoc_uri_destroy (uri);
   mongoc_client_pool_destroy (pool);

   _reset_handshake ();
}

/*
 * Append to the platform field a huge string
 * Make sure that it gets truncated
 */
static void
test_mongoc_handshake_too_big (void)
{
   mongoc_client_t *client;
   mock_server_t *server;
   mongoc_uri_t *uri;
   future_t *future;
   request_t *request;
   const bson_t *ismaster_doc;
   bson_iter_t iter;

   enum { BUFFER_SIZE = HANDSHAKE_MAX_SIZE };
   char big_string[BUFFER_SIZE];
   uint32_t len;
   const uint8_t *dummy;

   server = mock_server_new ();
   mock_server_run (server);

   _reset_handshake ();

   memset (big_string, 'a', BUFFER_SIZE - 1);
   big_string[BUFFER_SIZE - 1] = '\0';
   ASSERT (mongoc_handshake_data_append (NULL, NULL, big_string));

   uri = mongoc_uri_copy (mock_server_get_uri (server));
   /* avoid rare test timeouts */
   mongoc_uri_set_option_as_int32 (uri, MONGOC_URI_CONNECTTIMEOUTMS, 20000);
   client = mongoc_client_new_from_uri (uri);

   ASSERT (mongoc_client_set_appname (client, "my app"));

   /* Send a ping, mock server deals with it */
   future = future_client_command_simple (
      client, "admin", tmp_bson ("{'ping': 1}"), NULL, NULL, NULL);
   request = mock_server_receives_ismaster (server);

   /* Make sure the isMaster request has a handshake field, and it's not huge */
   ASSERT (request);
   ismaster_doc = request_get_doc (request, 0);
   ASSERT (ismaster_doc);
   ASSERT (bson_has_field (ismaster_doc, "isMaster"));
   ASSERT (bson_has_field (ismaster_doc, HANDSHAKE_FIELD));

   /* isMaster with handshake isn't too big */
   bson_iter_init_find (&iter, ismaster_doc, HANDSHAKE_FIELD);
   ASSERT (BSON_ITER_HOLDS_DOCUMENT (&iter));
   bson_iter_document (&iter, &len, &dummy);

   /* Should have truncated the platform field so it fits exactly */
   ASSERT (len == HANDSHAKE_MAX_SIZE);

   mock_server_replies_simple (request, "{'ok': 1}");
   request_destroy (request);

   request = mock_server_receives_command (
      server, "admin", MONGOC_QUERY_SLAVE_OK, "{'ping': 1}");

   mock_server_replies_simple (request, "{'ok': 1}");
   ASSERT (future_get_bool (future));

   future_destroy (future);
   request_destroy (request);
   mongoc_client_destroy (client);
   mongoc_uri_destroy (uri);
   mock_server_destroy (server);

   /* So later tests don't have "aaaaa..." as the md platform string */
   _reset_handshake ();
}

/* Test the case where we can't prevent the handshake doc being too big
 * and so we just don't send it */
static void
test_mongoc_handshake_cannot_send (void)
{
   mock_server_t *server;
   mongoc_uri_t *uri;
   mongoc_client_t *client;
   mongoc_client_pool_t *pool;
   request_t *request;
   const char *const server_reply = "{'ok': 1, 'ismaster': true}";
   const bson_t *request_doc;
   char big_string[HANDSHAKE_MAX_SIZE];
   mongoc_handshake_t *md;

   _reset_handshake ();
   capture_logs (true);

   /* Mess with our global handshake struct so the handshake doc will be
    * way too big */
   memset (big_string, 'a', HANDSHAKE_MAX_SIZE - 1);
   big_string[HANDSHAKE_MAX_SIZE - 1] = '\0';

   md = _mongoc_handshake_get ();
   bson_free (md->os_name);
   md->os_name = bson_strdup (big_string);

   server = mock_server_new ();
   mock_server_run (server);
   uri = mongoc_uri_copy (mock_server_get_uri (server));
   mongoc_uri_set_option_as_int32 (uri, MONGOC_URI_HEARTBEATFREQUENCYMS, 500);
   pool = mongoc_client_pool_new (uri);

   /* Pop a client to trigger the topology scanner */
   client = mongoc_client_pool_pop (pool);
   request = mock_server_receives_ismaster (server);

   /* Make sure the isMaster request DOESN'T have a handshake field: */
   ASSERT (request);
   request_doc = request_get_doc (request, 0);
   ASSERT (request_doc);
   ASSERT (bson_has_field (request_doc, "isMaster"));
   ASSERT (!bson_has_field (request_doc, HANDSHAKE_FIELD));

   mock_server_replies_simple (request, server_reply);
   request_destroy (request);

   /* Cause failure on client side */
   request = mock_server_receives_ismaster (server);
   ASSERT (request);
   mock_server_hangs_up (request);
   request_destroy (request);

   /* Make sure the isMaster request still DOESN'T have a handshake field
    * on subsequent heartbeats. */
   request = mock_server_receives_ismaster (server);
   ASSERT (request);
   request_doc = request_get_doc (request, 0);
   ASSERT (request_doc);
   ASSERT (bson_has_field (request_doc, "isMaster"));
   ASSERT (!bson_has_field (request_doc, HANDSHAKE_FIELD));

   mock_server_replies_simple (request, server_reply);
   request_destroy (request);

   /* cleanup */
   mongoc_client_pool_push (pool, client);

   mongoc_client_pool_destroy (pool);
   mongoc_uri_destroy (uri);
   mock_server_destroy (server);

   /* Reset again so the next tests don't have a handshake doc which
    * is too big */
   _reset_handshake ();
}

void
test_handshake_install (TestSuite *suite)
{
   TestSuite_Add (suite,
                  "/MongoDB/handshake/appname_in_uri",
                  test_mongoc_handshake_appname_in_uri);
   TestSuite_Add (suite,
                  "/MongoDB/handshake/appname_frozen_single",
                  test_mongoc_handshake_appname_frozen_single);
   TestSuite_Add (suite,
                  "/MongoDB/handshake/appname_frozen_pooled",
                  test_mongoc_handshake_appname_frozen_pooled);

   TestSuite_AddMockServerTest (suite,
                                "/MongoDB/handshake/success",
                                test_mongoc_handshake_data_append_success);
   TestSuite_Add (suite,
                  "/MongoDB/handshake/failure",
                  test_mongoc_handshake_data_append_after_cmd);
   TestSuite_AddMockServerTest (
      suite, "/MongoDB/handshake/too_big", test_mongoc_handshake_too_big);
   TestSuite_AddMockServerTest (suite,
                                "/MongoDB/handshake/cannot_send",
                                test_mongoc_handshake_cannot_send);
}
