#include <mongoc.h>
#include <mongoc-collection-private.h>
#include <mongoc-write-concern-private.h>
#include <mongoc-read-concern-private.h>

#include "TestSuite.h"
#include "test-libmongoc.h"
#include "mongoc-client-private.h"
#include "mongoc-database-private.h"
#include "mock_server/future-functions.h"
#include "mock_server/mock-server.h"
#include "test-conveniences.h"


static void
test_create_with_write_concern (void)
{
   mongoc_database_t *database;
   mongoc_collection_t *collection;
   mongoc_client_t *client;
   bson_error_t error = {0};
   mongoc_write_concern_t *bad_wc;
   mongoc_write_concern_t *good_wc;
   bool wire_version_5;
   bson_t *opts = NULL;
   char *dbname;
   char *name;

   capture_logs (true);
   opts = bson_new ();

   client = test_framework_client_new ();
   BSON_ASSERT (client);
   mongoc_client_set_error_api (client, 2);

   bad_wc = mongoc_write_concern_new ();
   good_wc = mongoc_write_concern_new ();

   wire_version_5 = test_framework_max_wire_version_at_least (5);

   dbname = gen_collection_name ("dbtest");
   database = mongoc_client_get_database (client, dbname);
   BSON_ASSERT (database);

   name = gen_collection_name ("create_collection");

   /* writeConcern that will not pass mongoc_write_concern_is_valid */
   bad_wc->wtimeout = -10;
   bson_reinit (opts);
   mongoc_write_concern_append_bad (bad_wc, opts);
   collection =
      mongoc_database_create_collection (database, name, opts, &error);
   ASSERT_ERROR_CONTAINS (error,
                          MONGOC_ERROR_COMMAND,
                          MONGOC_ERROR_COMMAND_INVALID_ARG,
                          "Invalid writeConcern");
   ASSERT (!collection);
   bad_wc->wtimeout = 0;
   error.code = 0;
   error.domain = 0;

   /* valid writeConcern on all configs */
   mongoc_write_concern_set_w (good_wc, 1);
   bson_reinit (opts);
   mongoc_write_concern_append (good_wc, opts);
   collection =
      mongoc_database_create_collection (database, name, opts, &error);
   ASSERT_OR_PRINT (collection, error);
   ASSERT (!error.code);
   ASSERT (!error.domain);

   ASSERT_OR_PRINT (mongoc_collection_drop (collection, &error), error);

   /* writeConcern that results in writeConcernError */
   bad_wc->wtimeout = 0;
   mongoc_write_concern_set_w (bad_wc, 99);
   if (!test_framework_is_mongos ()) { /* skip if sharded */
      bson_reinit (opts);
      mongoc_write_concern_append_bad (bad_wc, opts);
      mongoc_collection_destroy (collection);
      collection =
         mongoc_database_create_collection (database, name, opts, &error);

      if (wire_version_5) {
         ASSERT (!collection);
         if (test_framework_is_replset ()) { /* replica set */
            ASSERT_ERROR_CONTAINS (
               error, MONGOC_ERROR_WRITE_CONCERN, 100, "Write Concern error:");
         } else { /* standalone */
            ASSERT_CMPINT (error.domain, ==, MONGOC_ERROR_SERVER);
            ASSERT_CMPINT (error.code, ==, 2);
         }
      } else { /* if wire_version <= 4, no error */
         ASSERT_OR_PRINT (collection, error);
         ASSERT (!error.code);
         ASSERT (!error.domain);
         ASSERT_OR_PRINT (mongoc_collection_drop (collection, &error), error);
         mongoc_collection_destroy (collection);
      }
   }

   mongoc_database_destroy (database);
   bson_free (name);
   bson_free (dbname);
   bson_destroy (opts);
   mongoc_write_concern_destroy (good_wc);
   mongoc_write_concern_destroy (bad_wc);
   mongoc_client_destroy (client);
}


static void
test_copy (void)
{
   mongoc_database_t *database;
   mongoc_database_t *copy;
   mongoc_client_t *client;

   client = test_framework_client_new ();
   ASSERT (client);

   database = mongoc_client_get_database (client, "test");
   ASSERT (database);

   copy = mongoc_database_copy (database);
   ASSERT (copy);
   ASSERT (copy->client == database->client);
   ASSERT (strcmp (copy->name, database->name) == 0);

   mongoc_database_destroy (copy);
   mongoc_database_destroy (database);
   mongoc_client_destroy (client);
}

static void
test_has_collection (void)
{
   mongoc_collection_t *collection;
   mongoc_database_t *database;
   mongoc_client_t *client;
   bson_error_t error;
   char *name;
   bool r;
   bson_oid_t oid;
   bson_t b;

   client = test_framework_client_new ();
   BSON_ASSERT (client);

   name = gen_collection_name ("has_collection");
   collection = mongoc_client_get_collection (client, "test", name);
   BSON_ASSERT (collection);

   database = mongoc_client_get_database (client, "test");
   BSON_ASSERT (database);

   bson_init (&b);
   bson_oid_init (&oid, NULL);
   bson_append_oid (&b, "_id", 3, &oid);
   bson_append_utf8 (&b, "hello", 5, "world", 5);
   ASSERT_OR_PRINT (mongoc_collection_insert (
                       collection, MONGOC_INSERT_NONE, &b, NULL, &error),
                    error);
   bson_destroy (&b);

   r = mongoc_database_has_collection (database, name, &error);
   BSON_ASSERT (!error.domain);
   BSON_ASSERT (r);

   bson_free (name);
   mongoc_database_destroy (database);
   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
}


static void
test_command (void)
{
   mongoc_database_t *database;
   mongoc_client_t *client;
   mongoc_cursor_t *cursor;
   bson_error_t error;
   const bson_t *doc;
   bool r;
   bson_t cmd = BSON_INITIALIZER;
   bson_t reply;

   client = test_framework_client_new ();
   BSON_ASSERT (client);

   database = mongoc_client_get_database (client, "admin");

   /*
    * Test a known working command, "ping".
    */
   bson_append_int32 (&cmd, "ping", 4, 1);

   cursor = mongoc_database_command (
      database, MONGOC_QUERY_NONE, 0, 1, 0, &cmd, NULL, NULL);
   BSON_ASSERT (cursor);

   r = mongoc_cursor_next (cursor, &doc);
   BSON_ASSERT (r);
   BSON_ASSERT (doc);

   r = mongoc_cursor_next (cursor, &doc);
   BSON_ASSERT (!r);
   BSON_ASSERT (!doc);

   mongoc_cursor_destroy (cursor);


   /*
    * Test a non-existing command to ensure we get the failure.
    */
   bson_reinit (&cmd);
   bson_append_int32 (&cmd, "a_non_existing_command", -1, 1);

   r = mongoc_database_command_simple (database, &cmd, NULL, &reply, &error);
   BSON_ASSERT (!r);
   BSON_ASSERT (error.domain == MONGOC_ERROR_QUERY);
   BSON_ASSERT (error.code == MONGOC_ERROR_QUERY_COMMAND_NOT_FOUND);
   BSON_ASSERT (strstr (error.message, "a_non_existing_command"));

   bson_destroy (&reply);
   mongoc_database_destroy (database);
   mongoc_client_destroy (client);
   bson_destroy (&cmd);
}


static void
_test_db_command_read_prefs (bool simple, bool pooled)
{
   mock_server_t *server;
   mongoc_client_pool_t *pool = NULL;
   mongoc_client_t *client;
   mongoc_database_t *db;
   mongoc_read_prefs_t *secondary_pref;
   bson_t *cmd;
   future_t *future;
   bson_error_t error;
   request_t *request;
   mongoc_cursor_t *cursor;
   const bson_t *reply;

   /* mock mongos: easiest way to test that read preference is configured */
   server = mock_mongos_new (0);
   mock_server_run (server);

   if (pooled) {
      pool = mongoc_client_pool_new (mock_server_get_uri (server));
      client = mongoc_client_pool_pop (pool);
   } else {
      client = mongoc_client_new_from_uri (mock_server_get_uri (server));
   }

   db = mongoc_client_get_database (client, "db");
   secondary_pref = mongoc_read_prefs_new (MONGOC_READ_SECONDARY);
   mongoc_database_set_read_prefs (db, secondary_pref);
   cmd = tmp_bson ("{'foo': 1}");

   if (simple) {
      /* simple, without read preference */
      future = future_database_command_simple (db, cmd, NULL, NULL, &error);

      request = mock_server_receives_command (
         server, "db", MONGOC_QUERY_NONE, "{'foo': 1}");

      mock_server_replies_simple (request, "{'ok': 1}");
      ASSERT_OR_PRINT (future_get_bool (future), error);
      future_destroy (future);
      request_destroy (request);

      /* with read preference */
      future =
         future_database_command_simple (db, cmd, secondary_pref, NULL, &error);

      request = mock_server_receives_command (
         server,
         "db",
         MONGOC_QUERY_SLAVE_OK,
         "{'$query': {'foo': 1},"
         " '$readPreference': {'mode': 'secondary'}}");
      mock_server_replies_simple (request, "{'ok': 1}");
      ASSERT_OR_PRINT (future_get_bool (future), error);
      future_destroy (future);
      request_destroy (request);
   } else {
      /* not simple, no read preference */
      cursor = mongoc_database_command (
         db, MONGOC_QUERY_NONE, 0, 0, 0, cmd, NULL, NULL);
      future = future_cursor_next (cursor, &reply);
      request = mock_server_receives_command (
         server, "db", MONGOC_QUERY_NONE, "{'foo': 1}");

      mock_server_replies_simple (request, "{'ok': 1}");
      ASSERT (future_get_bool (future));
      future_destroy (future);
      request_destroy (request);
      mongoc_cursor_destroy (cursor);

      /* with read preference */
      cursor = mongoc_database_command (
         db, MONGOC_QUERY_NONE, 0, 0, 0, cmd, NULL, secondary_pref);
      future = future_cursor_next (cursor, &reply);
      request = mock_server_receives_command (
         server,
         "db",
         MONGOC_QUERY_SLAVE_OK,
         "{'$query': {'foo': 1},"
         " '$readPreference': {'mode': 'secondary'}}");

      mock_server_replies_simple (request, "{'ok': 1}");
      ASSERT (future_get_bool (future));
      future_destroy (future);
      request_destroy (request);
      mongoc_cursor_destroy (cursor);
   }

   mongoc_database_destroy (db);
   mongoc_read_prefs_destroy (secondary_pref);

   if (pooled) {
      mongoc_client_pool_push (pool, client);
      mongoc_client_pool_destroy (pool);
   } else {
      mongoc_client_destroy (client);
   }

   mock_server_destroy (server);
}


static void
test_db_command_simple_read_prefs_single (void)
{
   _test_db_command_read_prefs (true, false);
}


static void
test_db_command_simple_read_prefs_pooled (void)
{
   _test_db_command_read_prefs (true, true);
}


static void
test_db_command_read_prefs_single (void)
{
   _test_db_command_read_prefs (false, false);
}


static void
test_db_command_read_prefs_pooled (void)
{
   _test_db_command_read_prefs (false, true);
}


static void
test_drop (void)
{
   mongoc_client_t *client;
   mongoc_database_t *database;
   mongoc_collection_t *collection;
   bson_error_t error = {0};
   bson_t *opts = NULL;
   char *dbname;
   mongoc_write_concern_t *good_wc;
   mongoc_write_concern_t *bad_wc;
   bool wire_version_5;
   bool r;

   opts = bson_new ();
   client = test_framework_client_new ();
   BSON_ASSERT (client);
   mongoc_client_set_error_api (client, 2);

   bad_wc = mongoc_write_concern_new ();
   good_wc = mongoc_write_concern_new ();
   wire_version_5 = test_framework_max_wire_version_at_least (5);

   dbname = gen_collection_name ("db_drop_test");
   database = mongoc_client_get_database (client, dbname);

   /* MongoDB 3.2+ must create at least one replicated database before
    * dropDatabase will check writeConcern, see SERVER-25601 */
   collection = mongoc_database_get_collection (database, "collection");
   r = mongoc_collection_insert (
      collection, MONGOC_INSERT_NONE, tmp_bson ("{}"), NULL, &error);

   ASSERT_OR_PRINT (r, error);

   ASSERT_OR_PRINT (mongoc_database_drop (database, &error), error);
   BSON_ASSERT (!error.domain);
   BSON_ASSERT (!error.code);

   mongoc_database_destroy (database);

   /* invalid writeConcern */
   bad_wc->wtimeout = -10;
   database = mongoc_client_get_database (client, dbname);

   bson_reinit (opts);
   mongoc_write_concern_append_bad (bad_wc, opts);
   ASSERT (!mongoc_database_drop_with_opts (database, opts, &error));
   ASSERT_ERROR_CONTAINS (error,
                          MONGOC_ERROR_COMMAND,
                          MONGOC_ERROR_COMMAND_INVALID_ARG,
                          "Invalid writeConcern");
   bad_wc->wtimeout = 0;
   error.code = 0;
   error.domain = 0;

   /* valid writeConcern */
   mongoc_write_concern_set_w (good_wc, 1);

   bson_reinit (opts);
   mongoc_write_concern_append (good_wc, opts);
   ASSERT_OR_PRINT (mongoc_database_drop_with_opts (database, opts, &error),
                    error);
   BSON_ASSERT (!error.code);
   BSON_ASSERT (!error.domain);

   /* invalid writeConcern */
   mongoc_write_concern_set_w (bad_wc, 99);
   mongoc_database_destroy (database);

   if (!test_framework_is_mongos ()) { /* skip if sharded */
      database = mongoc_client_get_database (client, dbname);
      bson_reinit (opts);
      mongoc_write_concern_append_bad (bad_wc, opts);
      r = mongoc_database_drop_with_opts (database, opts, &error);
      if (wire_version_5) {
         ASSERT (!r);
         if (test_framework_is_replset ()) {
            ASSERT_ERROR_CONTAINS (
               error, MONGOC_ERROR_WRITE_CONCERN, 100, "Write Concern error:");
         } else { /* standalone */
            ASSERT_CMPINT (error.domain, ==, MONGOC_ERROR_SERVER);
            ASSERT_CMPINT (error.code, ==, 2);
         }
      } else { /* if wire_version <= 4, no error */
         ASSERT_OR_PRINT (r, error);
         ASSERT (!error.code);
         ASSERT (!error.domain);
         mongoc_database_destroy (database);
      }
   }

   bson_free (dbname);
   bson_destroy (opts);
   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
   mongoc_write_concern_destroy (good_wc);
   mongoc_write_concern_destroy (bad_wc);
}


static void
test_create_collection (void)
{
   mongoc_database_t *database;
   mongoc_collection_t *collection;
   mongoc_client_t *client;
   bson_error_t error = {0};
   bson_t options;
   bson_t storage_opts;
   bson_t wt_opts;

   char *dbname;
   char *name;

   client = test_framework_client_new ();
   BSON_ASSERT (client);

   dbname = gen_collection_name ("dbtest");
   database = mongoc_client_get_database (client, dbname);
   BSON_ASSERT (database);
   bson_free (dbname);

   bson_init (&options);
   BSON_APPEND_INT32 (&options, "size", 1234);
   BSON_APPEND_INT32 (&options, "max", 4567);
   BSON_APPEND_BOOL (&options, "capped", true);

   BSON_APPEND_DOCUMENT_BEGIN (&options, "storageEngine", &storage_opts);
   BSON_APPEND_DOCUMENT_BEGIN (&storage_opts, "wiredTiger", &wt_opts);
   BSON_APPEND_UTF8 (&wt_opts, "configString", "block_compressor=zlib");
   bson_append_document_end (&storage_opts, &wt_opts);
   bson_append_document_end (&options, &storage_opts);


   name = gen_collection_name ("create_collection");
   ASSERT_OR_PRINT (collection = mongoc_database_create_collection (
                       database, name, &options, &error),
                    error);

   bson_destroy (&options);
   bson_free (name);

   ASSERT_OR_PRINT (mongoc_collection_drop (collection, &error), error);

   ASSERT_OR_PRINT (mongoc_database_drop (database, &error), error);

   mongoc_collection_destroy (collection);
   mongoc_database_destroy (database);
   mongoc_client_destroy (client);
}

static void
test_get_collection_info (void)
{
   mongoc_database_t *database;
   mongoc_collection_t *collection;
   mongoc_client_t *client;
   mongoc_cursor_t *cursor;
   bson_error_t error = {0};
   bson_iter_t col_iter;
   bson_t capped_options = BSON_INITIALIZER;
   bson_t autoindexid_options = BSON_INITIALIZER;
   bson_t noopts_options = BSON_INITIALIZER;
   bson_t name_filter = BSON_INITIALIZER;
   const bson_t *doc;
   int num_infos = 0;

   const char *name;
   char *dbname;
   char *capped_name;
   char *autoindexid_name;
   char *noopts_name;

   client = test_framework_client_new ();
   BSON_ASSERT (client);

   dbname = gen_collection_name ("dbtest");
   database = mongoc_client_get_database (client, dbname);

   BSON_ASSERT (database);
   bson_free (dbname);

   capped_name = gen_collection_name ("capped");
   BSON_APPEND_BOOL (&capped_options, "capped", true);
   BSON_APPEND_INT32 (&capped_options, "size", 10000000);
   BSON_APPEND_INT32 (&capped_options, "max", 1024);

   autoindexid_name = gen_collection_name ("autoindexid");

   noopts_name = gen_collection_name ("noopts");

   collection = mongoc_database_create_collection (
      database, capped_name, &capped_options, &error);
   ASSERT_OR_PRINT (collection, error);
   mongoc_collection_destroy (collection);

   collection = mongoc_database_create_collection (
      database, autoindexid_name, &autoindexid_options, &error);
   ASSERT_OR_PRINT (collection, error);
   mongoc_collection_destroy (collection);

   collection = mongoc_database_create_collection (
      database, noopts_name, &noopts_options, &error);
   ASSERT_OR_PRINT (collection, error);
   mongoc_collection_destroy (collection);

   /* first we filter on collection name. */
   BSON_APPEND_UTF8 (&name_filter, "name", noopts_name);

   /* We only test with filters since get_collection_names will
    * test w/o filters for us. */

   /* Filter on an exact match of name */
   cursor = mongoc_database_find_collections (database, &name_filter, &error);
   BSON_ASSERT (cursor);
   BSON_ASSERT (!error.domain);
   BSON_ASSERT (!error.code);

   while (mongoc_cursor_next (cursor, &doc)) {
      if (bson_iter_init (&col_iter, doc) &&
          bson_iter_find (&col_iter, "name") &&
          BSON_ITER_HOLDS_UTF8 (&col_iter) &&
          (name = bson_iter_utf8 (&col_iter, NULL))) {
         ++num_infos;
         BSON_ASSERT (0 == strcmp (name, noopts_name));
      } else {
         BSON_ASSERT (false);
      }
   }

   BSON_ASSERT (1 == num_infos);

   mongoc_cursor_destroy (cursor);

   ASSERT_OR_PRINT (mongoc_database_drop (database, &error), error);
   BSON_ASSERT (!error.domain);
   BSON_ASSERT (!error.code);

   bson_free (capped_name);
   bson_free (noopts_name);
   bson_free (autoindexid_name);

   mongoc_database_destroy (database);
   mongoc_client_destroy (client);
}

static void
_test_get_collection_info_getmore (bool use_cmd)
{
   mock_server_t *server;
   mongoc_client_t *client;
   mongoc_database_t *database;
   future_t *future;
   request_t *request;
   char **names;

   server = mock_server_with_autoismaster (use_cmd ? WIRE_VERSION_FIND_CMD : 0);
   mock_server_run (server);
   client = mongoc_client_new_from_uri (mock_server_get_uri (server));
   database = mongoc_client_get_database (client, "db");
   future = future_database_get_collection_names (database, NULL);

   request = mock_server_receives_command (
      server, "db", MONGOC_QUERY_SLAVE_OK, "{'listCollections': 1}");

   if (use_cmd) {
      mock_server_replies_simple (request,
                                  "{'ok': 1,"
                                  " 'cursor': {"
                                  "    'id': {'$numberLong': '123'},"
                                  "    'ns': 'db.$cmd.listCollections',"
                                  "    'firstBatch': [{'name': 'a'}]}}");
      request_destroy (request);
      request = mock_server_receives_command (
         server,
         "db",
         MONGOC_QUERY_SLAVE_OK,
         "{'getMore': {'$numberLong': '123'},"
         " 'collection': '$cmd.listCollections'}");

      mock_server_replies_simple (request,
                                  "{'ok': 1,"
                                  " 'cursor': {"
                                  "    'id': {'$numberLong': '0'},"
                                  "    'ns': 'db.$cmd.listCollections',"
                                  "    'nextBatch': []}}");
      request_destroy (request);
   } else {
      /* "command not found" */
      mock_server_replies_simple (request, "{'ok': 0, 'code': 59}");
      request_destroy (request);

      request = mock_server_receives_query (server,
                                            "db.system.namespaces",
                                            MONGOC_QUERY_SLAVE_OK,
                                            0,
                                            0,
                                            NULL,
                                            NULL);
      mock_server_replies (request,
                           MONGOC_REPLY_NONE,
                           123 /* cursor id */,
                           0,
                           1,
                           "{'name': 'db.a'}");
      request_destroy (request);

      request =
         mock_server_receives_getmore (server, "db.system.namespaces", 0, 123);
      mock_server_replies (
         request, MONGOC_REPLY_NONE, 0 /* cursor id */, 0, 0, NULL);
      request_destroy (request);
   }

   names = future_get_char_ptr_ptr (future);
   BSON_ASSERT (names);
   ASSERT_CMPSTR (names[0], "a");

   bson_strfreev (names);
   future_destroy (future);
   mongoc_database_destroy (database);
   mongoc_client_destroy (client);
   mock_server_destroy (server);
}

static void
test_get_collection_info_op_getmore (void)
{
   _test_get_collection_info_getmore (false);
}

static void
test_get_collection_info_getmore_cmd (void)
{
   _test_get_collection_info_getmore (true);
}

static void
test_get_collection (void)
{
   mongoc_client_t *client;
   mongoc_database_t *database;
   mongoc_write_concern_t *wc;
   mongoc_read_concern_t *rc;
   mongoc_read_prefs_t *read_prefs;
   mongoc_collection_t *collection;

   client = test_framework_client_new ();
   BSON_ASSERT (client);

   database = mongoc_client_get_database (client, "test");

   wc = mongoc_write_concern_new ();
   mongoc_write_concern_set_w (wc, 2);
   mongoc_database_set_write_concern (database, wc);

   rc = mongoc_read_concern_new ();
   mongoc_read_concern_set_level (rc, "majority");
   mongoc_database_set_read_concern (database, rc);

   read_prefs = mongoc_read_prefs_new (MONGOC_READ_SECONDARY);
   mongoc_database_set_read_prefs (database, read_prefs);

   collection = mongoc_database_get_collection (database, "test");

   ASSERT_CMPINT32 (collection->write_concern->w, ==, 2);
   ASSERT_CMPSTR (collection->read_concern->level, "majority");
   ASSERT_CMPINT (collection->read_prefs->mode, ==, MONGOC_READ_SECONDARY);

   mongoc_collection_destroy (collection);
   mongoc_read_prefs_destroy (read_prefs);
   mongoc_read_concern_destroy (rc);
   mongoc_write_concern_destroy (wc);
   mongoc_database_destroy (database);
   mongoc_client_destroy (client);
}

static void
test_get_collection_names (void)
{
   mongoc_database_t *database;
   mongoc_collection_t *collection;
   mongoc_client_t *client;
   bson_error_t error = {0};
   bson_t options;
   int namecount = 0;

   char **names;
   char **name;
   char *curname;

   char *dbname;
   char *name1;
   char *name2;
   char *name3;
   char *name4;
   char *name5;
   const char *system_prefix = "system.";

   client = test_framework_client_new ();
   BSON_ASSERT (client);

   dbname = gen_collection_name ("dbtest");
   database = mongoc_client_get_database (client, dbname);

   BSON_ASSERT (database);
   bson_free (dbname);

   bson_init (&options);

   name1 = gen_collection_name ("name1");
   name2 = gen_collection_name ("name2");
   name3 = gen_collection_name ("name3");
   name4 = gen_collection_name ("name4");
   name5 = gen_collection_name ("name5");

   collection =
      mongoc_database_create_collection (database, name1, &options, &error);
   BSON_ASSERT (collection);
   mongoc_collection_destroy (collection);

   collection =
      mongoc_database_create_collection (database, name2, &options, &error);
   BSON_ASSERT (collection);
   mongoc_collection_destroy (collection);

   collection =
      mongoc_database_create_collection (database, name3, &options, &error);
   BSON_ASSERT (collection);
   mongoc_collection_destroy (collection);

   collection =
      mongoc_database_create_collection (database, name4, &options, &error);
   BSON_ASSERT (collection);
   mongoc_collection_destroy (collection);

   collection =
      mongoc_database_create_collection (database, name5, &options, &error);
   BSON_ASSERT (collection);
   mongoc_collection_destroy (collection);

   names = mongoc_database_get_collection_names (database, &error);
   BSON_ASSERT (!error.domain);
   BSON_ASSERT (!error.code);

   for (name = names; *name; ++name) {
      /* inefficient, but OK for a unit test. */
      curname = *name;

      if (0 == strcmp (curname, name1) || 0 == strcmp (curname, name2) ||
          0 == strcmp (curname, name3) || 0 == strcmp (curname, name4) ||
          0 == strcmp (curname, name5)) {
         ++namecount;
      } else if (0 ==
                 strncmp (curname, system_prefix, strlen (system_prefix))) {
         /* Collections prefixed with 'system.' are system collections */
      } else {
         BSON_ASSERT (false);
      }

      bson_free (curname);
   }

   BSON_ASSERT (namecount == 5);

   bson_free (name1);
   bson_free (name2);
   bson_free (name3);
   bson_free (name4);
   bson_free (name5);

   bson_free (names);

   ASSERT_OR_PRINT (mongoc_database_drop (database, &error), error);
   BSON_ASSERT (!error.domain);
   BSON_ASSERT (!error.code);

   mongoc_database_destroy (database);
   mongoc_client_destroy (client);
}

static void
test_get_collection_names_error (void)
{
   mongoc_database_t *database;
   mongoc_client_t *client;
   mock_server_t *server;
   bson_error_t error = {0};
   bson_t b = BSON_INITIALIZER;
   future_t *future;
   request_t *request;
   char **names;

   capture_logs (true);

   server = mock_server_new ();
   mock_server_auto_ismaster (server,
                              "{'ismaster': true,"
                              " 'maxWireVersion': 3}");
   mock_server_run (server);
   client = mongoc_client_new_from_uri (mock_server_get_uri (server));

   database = mongoc_client_get_database (client, "test");
   future = future_database_get_collection_names (database, &error);
   request = mock_server_receives_command (
      server, "test", MONGOC_QUERY_SLAVE_OK, "{'listCollections': 1}");
   mock_server_hangs_up (request);
   names = future_get_char_ptr_ptr (future);
   BSON_ASSERT (!names);
   ASSERT_CMPINT (MONGOC_ERROR_STREAM, ==, error.domain);
   ASSERT_CMPINT (MONGOC_ERROR_STREAM_SOCKET, ==, error.code);

   request_destroy (request);
   future_destroy (future);
   mongoc_database_destroy (database);
   mongoc_client_destroy (client);
   mock_server_destroy (server);
   bson_destroy (&b);
}

static void
test_get_default_database (void)
{
   /* default database is "db_name" */
   mongoc_client_t *client = mongoc_client_new ("mongodb://host/db_name");
   mongoc_database_t *db = mongoc_client_get_default_database (client);

   BSON_ASSERT (!strcmp ("db_name", mongoc_database_get_name (db)));

   mongoc_database_destroy (db);
   mongoc_client_destroy (client);

   /* no default database */
   client = mongoc_client_new ("mongodb://host/");
   db = mongoc_client_get_default_database (client);

   BSON_ASSERT (!db);

   mongoc_client_destroy (client);
}

void
test_database_install (TestSuite *suite)
{
   TestSuite_AddLive (suite,
                      "/Database/create_with_write_concern",
                      test_create_with_write_concern);
   TestSuite_AddLive (suite, "/Database/copy", test_copy);
   TestSuite_AddLive (suite, "/Database/has_collection", test_has_collection);
   TestSuite_AddLive (suite, "/Database/command", test_command);
   TestSuite_AddMockServerTest (suite,
                                "/Database/command/read_prefs/simple/single",
                                test_db_command_simple_read_prefs_single);
   TestSuite_AddMockServerTest (suite,
                                "/Database/command/read_prefs/simple/pooled",
                                test_db_command_simple_read_prefs_pooled);
   TestSuite_AddMockServerTest (suite,
                                "/Database/command/read_prefs/single",
                                test_db_command_read_prefs_single);
   TestSuite_AddMockServerTest (suite,
                                "/Database/command/read_prefs/pooled",
                                test_db_command_read_prefs_pooled);
   TestSuite_AddLive (suite, "/Database/drop", test_drop);
   TestSuite_AddLive (
      suite, "/Database/create_collection", test_create_collection);
   TestSuite_AddLive (
      suite, "/Database/get_collection_info", test_get_collection_info);
   TestSuite_AddMockServerTest (suite,
                                "/Database/get_collection/op_getmore",
                                test_get_collection_info_op_getmore);
   TestSuite_AddMockServerTest (suite,
                                "/Database/get_collection/getmore_cmd",
                                test_get_collection_info_getmore_cmd);
   TestSuite_AddLive (suite, "/Database/get_collection", test_get_collection);
   TestSuite_AddLive (
      suite, "/Database/get_collection_names", test_get_collection_names);
   TestSuite_AddMockServerTest (suite,
                                "/Database/get_collection_names_error",
                                test_get_collection_names_error);
   TestSuite_Add (
      suite, "/Database/get_default_database", test_get_default_database);
}
