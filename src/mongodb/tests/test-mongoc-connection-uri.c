#include <mongoc.h>
#include <mongoc-uri-private.h>

#include "json-test.h"
#include "test-libmongoc.h"
#include "mongoc-read-concern-private.h"


static void
run_uri_test (const char *uri_string,
              bool valid,
              const bson_t *hosts,
              const bson_t *auth,
              const bson_t *options)
{
   mongoc_uri_t *uri = mongoc_uri_new (uri_string);

   if (valid) {
      BSON_ASSERT (uri);
   } else {
      BSON_ASSERT (!uri);
      return;
   }

   if (!bson_empty0 (hosts)) {
      const mongoc_host_list_t *hl;
      bson_iter_t iter;
      bson_iter_t host_iter;

      for (bson_iter_init (&iter, hosts);
           bson_iter_next (&iter) && bson_iter_recurse (&iter, &host_iter);) {
         const char *host = "localhost";
         int port = 27017;
         bool ok = false;

         if (bson_iter_find (&host_iter, "host") &&
             BSON_ITER_HOLDS_UTF8 (&host_iter)) {
            host = bson_iter_utf8 (&host_iter, NULL);
         }
         if (bson_iter_find (&host_iter, "port") &&
             BSON_ITER_HOLDS_INT (&host_iter)) {
            port = bson_iter_as_int64 (&host_iter);
         }

         for (hl = mongoc_uri_get_hosts (uri); hl; hl = hl->next) {
            if (!strcmp (host, hl->host) && port == hl->port) {
               ok = true;
               break;
            }
         }

         if (!ok) {
            fprintf (stderr,
                     "Could not find '%s':%d in uri '%s'\n",
                     host,
                     port,
                     mongoc_uri_get_string (uri));
            BSON_ASSERT (0);
         }
      }
   }

   if (!bson_empty0 (auth)) {
      const char *auth_source = mongoc_uri_get_auth_source (uri);
      const char *username = mongoc_uri_get_username (uri);
      const char *password = mongoc_uri_get_password (uri);
      bson_iter_t iter;

      if (bson_iter_init_find (&iter, auth, "username") &&
          BSON_ITER_HOLDS_UTF8 (&iter)) {
         ASSERT_CMPSTR (username, bson_iter_utf8 (&iter, NULL));
      }

      if (bson_iter_init_find (&iter, auth, "password") &&
          BSON_ITER_HOLDS_UTF8 (&iter)) {
         ASSERT_CMPSTR (password, bson_iter_utf8 (&iter, NULL));
      }

      if (bson_iter_init_find (&iter, auth, "db") &&
          BSON_ITER_HOLDS_UTF8 (&iter)) {
         ASSERT_CMPSTR (auth_source, bson_iter_utf8 (&iter, NULL));
      }
   }
   if (options) {
      const mongoc_read_concern_t *rc;
      bson_t all = BSON_INITIALIZER;
      bson_iter_t iter;
      bson_iter_t key_iter;

      bson_concat (&all, mongoc_uri_get_options (uri));
      bson_concat (&all, mongoc_uri_get_credentials (uri));
      rc = mongoc_uri_get_read_concern (uri);
      if (!mongoc_read_concern_is_default (rc)) {
         BSON_APPEND_UTF8 (
            &all, "readconcernlevel", mongoc_read_concern_get_level (rc));
      }

      for (bson_iter_init (&iter, options); bson_iter_next (&iter);) {
         ASSERT (
            bson_iter_init_find_case (&key_iter, &all, bson_iter_key (&iter)));
         ASSERT_CMPSTR (bson_iter_utf8 (&key_iter, 0),
                        bson_iter_utf8 (&iter, 0));
      }
      bson_destroy (&all);
   }

   if (uri) {
      mongoc_uri_destroy (uri);
   }
}

static void
test_connection_uri_cb (bson_t *scenario)
{
   bson_iter_t iter;
   bson_iter_t descendent;
   bson_iter_t tests_iter;
   bson_iter_t warning_iter;
   const char *uri_string = NULL;
   bson_t hosts = BSON_INITIALIZER;
   bson_t auth = BSON_INITIALIZER;
   bson_t options = BSON_INITIALIZER;
   bool valid;
   int c = 0;

   BSON_ASSERT (scenario);


   BSON_ASSERT (bson_iter_init_find (&iter, scenario, "tests"));
   BSON_ASSERT (BSON_ITER_HOLDS_ARRAY (&iter));
   bson_iter_recurse (&iter, &tests_iter);

   while (bson_iter_next (&tests_iter)) {
      bson_t test_case;

      bson_iter_bson (&tests_iter, &test_case);
      c++;

      if (test_suite_debug_output ()) {
         bson_iter_t test_case_iter;

         ASSERT (bson_iter_recurse (&tests_iter, &test_case_iter));
         if (bson_iter_find (&test_case_iter, "description")) {
            const char *description = bson_iter_utf8 (&test_case_iter, NULL);
            bson_iter_find_case (&test_case_iter, "uri");

            printf ("  - %s: '%s'\n",
                    description,
                    bson_iter_utf8 (&test_case_iter, 0));
            fflush (stdout);
         } else {
            fprintf (stderr, "Couldn't find `description` field in testcase\n");
            BSON_ASSERT (0);
         }
      }

      uri_string = bson_lookup_utf8 (&test_case, "uri");
      bson_lookup_doc_null_ok (&test_case, "hosts", &hosts);
      bson_lookup_doc_null_ok (&test_case, "auth", &auth);
      bson_lookup_doc_null_ok (&test_case, "options", &options);

      valid = bson_lookup_bool (&test_case, "valid", true);
      capture_logs (true);
      run_uri_test (uri_string, valid, &hosts, &auth, &options);


      bson_iter_init (&warning_iter, &test_case);

      if (bson_iter_find_descendant (&warning_iter, "warning", &descendent) &&
          BSON_ITER_HOLDS_BOOL (&descendent)) {
         if (bson_iter_as_bool (&descendent)) {
            ASSERT_CAPTURED_LOG ("mongoc_uri", MONGOC_LOG_LEVEL_WARNING, "");
         } else {
            ASSERT_NO_CAPTURED_LOGS ("mongoc_uri");
         }
      }

      bson_reinit (&hosts);
      bson_reinit (&auth);
      bson_reinit (&options);
   }
}


static void
test_all_spec_tests (TestSuite *suite)
{
   char resolved[PATH_MAX];

   ASSERT (realpath (JSON_DIR "/connection_uri", resolved));
   install_json_test_suite (suite, resolved, &test_connection_uri_cb);
}


void
test_connection_uri_install (TestSuite *suite)
{
   test_all_spec_tests (suite);
}
