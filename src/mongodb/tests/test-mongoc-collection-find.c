#include <mongoc.h>
#include <mongoc-cursor-private.h>
#include "mongoc-uri-private.h"
#include "mongoc-client-private.h"

#include "TestSuite.h"
#include "test-libmongoc.h"
#include "test-conveniences.h"
#include "mock_server/mock-server.h"
#include "mock_server/future.h"
#include "mock_server/future-functions.h"


typedef struct {
   /* if do_live is true (the default), actually query the server using the
    * appropriate wire protocol: either OP_QUERY or a "find" command */
   bool do_live;
   bool requires_wire_version_4;
   const char *docs;
   bson_t *docs_bson;
   const char *query_input;
   bson_t *query_bson;
   const char *fields;
   bson_t *fields_bson;
   const char *expected_find_command;
   const char *expected_op_query;
   int32_t n_return;
   const char *expected_result;
   bson_t *expected_result_bson;
   uint32_t skip;
   int32_t limit;
   uint32_t batch_size;
   mongoc_query_flags_t flags;
   mongoc_read_prefs_t *read_prefs;
   const char *filename;
   int lineno;
   const char *funcname;
   uint32_t n_results;
} test_collection_find_t;


#define TEST_COLLECTION_FIND_INIT \
   {                              \
      true, false                 \
   }


static void
_insert_test_docs (mongoc_collection_t *collection, const bson_t *docs)
{
   bson_iter_t iter;
   uint32_t len;
   const uint8_t *data;
   bson_t doc;
   bool r;
   bson_error_t error;

   bson_iter_init (&iter, docs);
   while (bson_iter_next (&iter)) {
      bson_iter_document (&iter, &len, &data);
      bson_init_static (&doc, data, len);
      r = mongoc_collection_insert (
         collection, MONGOC_INSERT_NONE, &doc, NULL, &error);
      ASSERT_OR_PRINT (r, error);
   }
}


static void
_check_cursor (mongoc_cursor_t *cursor, test_collection_find_t *test_data)
{
   const bson_t *doc;
   bson_t actual_result = BSON_INITIALIZER;
   char str[16];
   const char *key;
   uint32_t i = 0;
   bson_error_t error;

   while (mongoc_cursor_next (cursor, &doc)) {
      bson_uint32_to_string (i, &key, str, sizeof str);
      bson_append_document (&actual_result, key, -1, doc);
      i++;
   }

   ASSERT_OR_PRINT (!mongoc_cursor_error (cursor, &error), error);

   if (i != test_data->n_results) {
      fprintf (stderr, "expect %d results, got %d\n", test_data->n_results, i);
      abort ();
   }

   ASSERT (match_json (&actual_result,
                       false /* is_command */,
                       test_data->filename,
                       test_data->lineno,
                       test_data->funcname,
                       test_data->expected_result));

   bson_destroy (&actual_result);
}


static void
_test_collection_find_live (test_collection_find_t *test_data)

{
   mongoc_client_t *client;
   mongoc_database_t *database;
   char *collection_name;
   mongoc_collection_t *collection;
   char *drop_cmd;
   bool r;
   bson_error_t error;
   mongoc_cursor_t *cursor;

   client = test_framework_client_new ();
   database = mongoc_client_get_database (client, "test");
   collection_name = gen_collection_name ("test");
   collection = mongoc_database_create_collection (
      database,
      collection_name,
      tmp_bson ("{'capped': true, 'size': 10000}"),
      &error);

   ASSERT_OR_PRINT (collection, error);

   _insert_test_docs (collection, test_data->docs_bson);

   cursor = mongoc_collection_find (collection,
                                    MONGOC_QUERY_NONE,
                                    test_data->skip,
                                    test_data->limit,
                                    test_data->batch_size,
                                    test_data->query_bson,
                                    test_data->fields_bson,
                                    test_data->read_prefs);

   _check_cursor (cursor, test_data);

   drop_cmd = bson_strdup_printf ("{'drop': '%s'}", collection_name);
   r = mongoc_client_command_simple (
      client, "test", tmp_bson (drop_cmd), NULL, NULL, &error);
   ASSERT_OR_PRINT (r, error);

   bson_free (drop_cmd);
   mongoc_cursor_destroy (cursor);
   mongoc_collection_destroy (collection);
   mongoc_database_destroy (database);
   bson_free (collection_name);
   mongoc_client_destroy (client);
}


typedef request_t *(*check_request_fn_t) (mock_server_t *server,
                                          test_collection_find_t *test_data);


typedef void (*reply_fn_t) (request_t *request,
                            test_collection_find_t *test_data);


/*--------------------------------------------------------------------------
 *
 * _test_collection_op_query_or_find_command --
 *
 *       Start a mock server with @max_wire_version, connect a client, and
 *       execute @test_data->query. Use the @check_request_fn callback to
 *       verify the client formatted the query correctly, and @reply_fn to
 *       response to the client. Check that the client cursor's results
 *       match @test_data->expected_result.
 *
 *--------------------------------------------------------------------------
 */

static void
_test_collection_op_query_or_find_command (test_collection_find_t *test_data,
                                           check_request_fn_t check_request_fn,
                                           reply_fn_t reply_fn,
                                           int32_t max_wire_version)
{
   mock_server_t *server;
   mongoc_client_t *client;
   mongoc_collection_t *collection;
   mongoc_cursor_t *cursor;
   bson_error_t error;
   future_t *future;
   request_t *request;
   const bson_t *doc;
   bool cursor_next_result;
   bson_t actual_result = BSON_INITIALIZER;
   char str[16];
   const char *key;
   uint32_t i = 0;

   if (!TestSuite_CheckMockServerAllowed ()) {
      return;
   }

   server = mock_server_with_autoismaster (max_wire_version);
   mock_server_run (server);
   client = mongoc_client_new_from_uri (mock_server_get_uri (server));
   collection = mongoc_client_get_collection (client, "db", "collection");
   cursor = mongoc_collection_find (collection,
                                    test_data->flags,
                                    test_data->skip,
                                    test_data->limit,
                                    test_data->batch_size,
                                    test_data->query_bson,
                                    test_data->fields_bson,
                                    test_data->read_prefs);

   ASSERT_OR_PRINT (!mongoc_cursor_error (cursor, &error), error);
   future = future_cursor_next (cursor, &doc);
   request = check_request_fn (server, test_data);
   ASSERT (request);
   reply_fn (request, test_data);

   cursor_next_result = future_get_bool (future);
   /* did we expect at least one result? */
   ASSERT (cursor_next_result == (test_data->n_results > 0));
   BSON_ASSERT (!mongoc_cursor_error (cursor, NULL));

   if (cursor_next_result) {
      bson_append_document (&actual_result, "0", -1, doc);
      i++;

      /* check remaining results */
      while (mongoc_cursor_next (cursor, &doc)) {
         bson_uint32_to_string (i, &key, str, sizeof str);
         bson_append_document (&actual_result, key, -1, doc);
         i++;
      }

      BSON_ASSERT (!mongoc_cursor_error (cursor, NULL));
   }

   if (i != test_data->n_results) {
      fprintf (
         stderr, "Expected %d results, got %d\n", test_data->n_results, i);
      abort ();
   }

   ASSERT (match_json (&actual_result,
                       false /* is_command */,
                       test_data->filename,
                       test_data->lineno,
                       test_data->funcname,
                       test_data->expected_result));

   request_destroy (request);
   future_destroy (future);
   mongoc_cursor_destroy (cursor);
   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
   mock_server_destroy (server);
}


static request_t *
_check_op_query (mock_server_t *server, test_collection_find_t *test_data)
{
   mongoc_query_flags_t flags;

   flags = test_data->flags | MONGOC_QUERY_SLAVE_OK;

   return mock_server_receives_query (server,
                                      "db.collection",
                                      flags,
                                      test_data->skip,
                                      test_data->n_return,
                                      test_data->expected_op_query,
                                      test_data->fields);
}


static void
_reply_to_op_query (request_t *request, test_collection_find_t *test_data)
{
   bson_t *docs;
   int i;
   bson_iter_t iter;
   uint32_t len;
   const uint8_t *data;

   docs = bson_malloc (test_data->n_results * sizeof (bson_t));
   bson_iter_init (&iter, test_data->expected_result_bson);

   for (i = 0; i < test_data->n_results; i++) {
      bson_iter_next (&iter);
      bson_iter_document (&iter, &len, &data);
      bson_init_static (&docs[i], data, len);
   }

   mock_server_reply_multi (request,
                            MONGOC_REPLY_NONE,
                            docs,
                            test_data->n_results,
                            0 /* cursor_id */);

   bson_free (docs);
}


static void
_test_collection_op_query (test_collection_find_t *test_data)
{
   _test_collection_op_query_or_find_command (
      test_data, _check_op_query, _reply_to_op_query, 3 /* wire version */);
}


static request_t *
_check_find_command (mock_server_t *server, test_collection_find_t *test_data)
{
   /* Server Selection Spec: all queries to standalone set slaveOk.
    *
    * Find, getMore And killCursors Commands Spec: "When sending a find command
    * rather than a legacy OP_QUERY find only the slaveOk flag is honored".
    */
   return mock_server_receives_command (
      server, "db", MONGOC_QUERY_SLAVE_OK, test_data->expected_find_command);
}


static void
_reply_to_find_command (request_t *request, test_collection_find_t *test_data)
{
   const char *result_json;
   char *reply_json;

   result_json = test_data->expected_result ? test_data->expected_result : "[]";

   reply_json = bson_strdup_printf ("{'ok': 1,"
                                    " 'cursor': {"
                                    "    'id': 0,"
                                    "    'ns': 'db.collection',"
                                    "    'firstBatch': %s}}",
                                    result_json);

   mock_server_replies_simple (request, reply_json);

   bson_free (reply_json);
}


static void
_test_collection_find_command (test_collection_find_t *test_data)

{
   _test_collection_op_query_or_find_command (test_data,
                                              _check_find_command,
                                              _reply_to_find_command,
                                              4 /* max wire version */);
}


static void
_test_collection_find (test_collection_find_t *test_data)
{
   /* catch typos in tests' setup */
   if (test_data->query_input) {
      BSON_ASSERT (test_data->requires_wire_version_4 ||
                   test_data->expected_op_query);
   }

   BSON_ASSERT (test_data->expected_find_command);

   test_data->docs_bson = tmp_bson (test_data->docs);
   test_data->query_bson = tmp_bson (test_data->query_input);
   test_data->fields_bson =
      test_data->fields ? tmp_bson (test_data->fields) : NULL;
   test_data->expected_result_bson = tmp_bson (test_data->expected_result);
   test_data->n_results = bson_count_keys (test_data->expected_result_bson);

   if (test_data->do_live && (!test_data->requires_wire_version_4 ||
                              test_framework_max_wire_version_at_least (4))) {
      _test_collection_find_live (test_data);
   }

   if (!test_data->requires_wire_version_4) {
      _test_collection_op_query (test_data);
   }

   _test_collection_find_command (test_data);
}


static void
test_dollar_query (void)
{
   test_collection_find_t test_data = TEST_COLLECTION_FIND_INIT;
   test_data.docs = "[{'_id': 1}, {'_id': 2}]";
   test_data.query_input = "{'$query': {'_id': 1}}";
   test_data.expected_op_query = "{'_id': 1}";
   test_data.expected_find_command =
      "{'find': 'collection', 'filter': {'_id': 1}}";
   test_data.expected_result = "[{'_id': 1}]";
   _test_collection_find (&test_data);
}


static void
test_dollar_or (void)
{
   test_collection_find_t test_data = TEST_COLLECTION_FIND_INIT;

   test_data.docs = "[{'_id': 1}, {'_id': 2}, {'_id': 3}]";
   test_data.query_input = "{'$or': [{'_id': 1}, {'_id': 3}]}";
   test_data.expected_op_query = test_data.query_input;
   test_data.expected_find_command =
      "{'find': 'collection', 'filter': {'$or': [{'_id': 1}, {'_id': 3}]}}";

   test_data.expected_result = "[{'_id': 1}, {'_id': 3}]";
   _test_collection_find (&test_data);
}


static void
test_mixed_dollar_nondollar (void)
{
   test_collection_find_t test_data = TEST_COLLECTION_FIND_INIT;

   test_data.docs = "[{'a': 1}, {'a': 1, 'b': 2}, {'a': 2}]";
   test_data.query_input = "{'a': 1, '$or': [{'b': 1}, {'b': 2}]}";
   test_data.expected_op_query = test_data.query_input;
   test_data.expected_find_command =
      "{'find': 'collection', 'filter': {'a': 1, '$or': [{'b': 1}, {'b': 2}]}}";

   test_data.expected_result = "[{'a': 1, 'b': 2}]";
   _test_collection_find (&test_data);
}


/* test that we can query for a document by a key named "filter" */
static void
test_key_named_filter (void)
{
   test_collection_find_t test_data = TEST_COLLECTION_FIND_INIT;
   test_data.docs = "[{'_id': 1, 'filter': 1}, {'_id': 2, 'filter': 2}]";
   test_data.query_input = "{'filter': 2}";
   test_data.expected_op_query = "{'filter': 2}";
   test_data.expected_find_command =
      "{'find': 'collection', 'filter': {'filter': 2}}";
   test_data.expected_result = "[{'_id': 2, 'filter': 2}]";
   _test_collection_find (&test_data);
}


/* test that we can query for a document by a key named "filter" using $query */
static void
test_key_named_filter_with_dollar_query (void)
{
   test_collection_find_t test_data = TEST_COLLECTION_FIND_INIT;
   test_data.docs = "[{'_id': 1, 'filter': 1}, {'_id': 2, 'filter': 2}]";
   test_data.query_input = "{'$query': {'filter': 2}}";
   test_data.expected_op_query = "{'filter': 2}";
   test_data.expected_find_command =
      "{'find': 'collection', 'filter': {'filter': 2}}";
   test_data.expected_result = "[{'_id': 2, 'filter': 2}]";
   _test_collection_find (&test_data);
}


/* test 'filter': {'i': 2} */
static void
test_subdoc_named_filter (void)
{
   test_collection_find_t test_data = TEST_COLLECTION_FIND_INIT;
   test_data.docs =
      "[{'_id': 1, 'filter': {'i': 1}}, {'_id': 2, 'filter': {'i': 2}}]";
   test_data.query_input = "{'filter': {'i': 2}}";
   test_data.expected_op_query = test_data.query_input;
   test_data.expected_find_command =
      "{'find': 'collection', 'filter': {'filter': {'i': 2}}}";
   test_data.expected_result = "[{'_id': 2, 'filter': {'i': 2}}]";

   _test_collection_find (&test_data);
}


/* test '$query': {'filter': {'i': 2}} */
static void
test_subdoc_named_filter_with_dollar_query (void)
{
   test_collection_find_t test_data = TEST_COLLECTION_FIND_INIT;
   test_data.docs =
      "[{'_id': 1, 'filter': {'i': 1}}, {'_id': 2, 'filter': {'i': 2}}]";
   test_data.query_input = "{'$query': {'filter': {'i': 2}}}";
   test_data.expected_op_query = "{'filter': {'i': 2}}";
   test_data.expected_find_command =
      "{'find': 'collection', 'filter': {'filter': {'i': 2}}}";
   test_data.expected_result = "[{'_id': 2, 'filter': {'i': 2}}]";
   _test_collection_find (&test_data);
}


/* test future-compatibility with a new server's find command options */
static void
test_newoption (void)
{
   test_collection_find_t test_data = TEST_COLLECTION_FIND_INIT;
   test_data.query_input = "{'$query': {'_id': 1}, '$newOption': true}";
   test_data.expected_op_query = test_data.query_input;
   test_data.expected_find_command =
      "{'find': 'collection', 'filter': {'_id': 1}, 'newOption': true}";

   /* won't work today */
   test_data.do_live = false;

   _test_collection_find (&test_data);
}


static void
test_orderby (void)
{
   test_collection_find_t test_data = TEST_COLLECTION_FIND_INIT;
   test_data.docs = "[{'_id': 1}, {'_id': 2}]";
   test_data.query_input = "{'$query': {}, '$orderby': {'_id': -1}}";
   test_data.expected_op_query = test_data.query_input;
   test_data.expected_find_command =
      "{'find': 'collection', 'filter': {}, 'sort': {'_id': -1}}";
   test_data.expected_result = "[{'_id': 2}, {'_id': 1}]";
   _test_collection_find (&test_data);
}


static void
test_fields (void)
{
   test_collection_find_t test_data = TEST_COLLECTION_FIND_INIT;
   test_data.docs = "[{'_id': 1, 'a': 1, 'b': 2}]";
   test_data.fields = "{'_id': 0, 'b': 1}";
   test_data.expected_find_command =
      "{'find': 'collection', 'filter': {}, 'projection': {'_id': 0, 'b': 1}}";
   test_data.expected_result = "[{'b': 2}]";
   _test_collection_find (&test_data);
}


static void
test_int_modifiers (void)
{
   const char *modifiers[] = {
      "maxScan", "maxTimeMS",
   };

   const char *mod;
   size_t i;
   char *query;
   char *find_command;
   test_collection_find_t test_data = TEST_COLLECTION_FIND_INIT;

   test_data.expected_result = test_data.docs = "[{'_id': 1}]";

   for (i = 0; i < sizeof (modifiers) / sizeof (const char *); i++) {
      mod = modifiers[i];
      query = bson_strdup_printf ("{'$query': {}, '$%s': 9999}", mod);

      /* find command has same modifier, without the $-prefix */
      find_command = bson_strdup_printf (
         "{'find': 'collection', 'filter': {}, '%s': 9999}", mod);

      test_data.expected_op_query = test_data.query_input = query;
      test_data.expected_find_command = find_command;
      _test_collection_find (&test_data);
      bson_free (query);
      bson_free (find_command);
   }
}


static void
test_index_spec_modifiers (void)
{
   /* don't include $max, it needs a slightly different argument to succeed */
   const char *modifiers[] = {
      "hint", "min",
   };

   const char *mod;
   size_t i;
   char *query;
   char *find_command;
   test_collection_find_t test_data = TEST_COLLECTION_FIND_INIT;

   test_data.expected_result = test_data.docs = "[{'_id': 1}]";

   for (i = 0; i < sizeof (modifiers) / sizeof (const char *); i++) {
      mod = modifiers[i];
      query = bson_strdup_printf ("{'$query': {}, '$%s': {'_id': 1}}", mod);

      /* find command has same modifier, without the $-prefix */
      find_command = bson_strdup_printf (
         "{'find': 'collection', 'filter': {}, '%s': {'_id': 1}}", mod);

      test_data.expected_op_query = test_data.query_input = query;
      test_data.expected_find_command = find_command;
      _test_collection_find (&test_data);

      bson_free (query);
      bson_free (find_command);
   }
}


static void
test_comment (void)
{
   test_collection_find_t test_data = TEST_COLLECTION_FIND_INIT;
   test_data.docs = "[{'_id': 1}]";
   test_data.query_input = "{'$query': {}, '$comment': 'hi'}";
   test_data.expected_op_query = test_data.query_input;
   test_data.expected_find_command =
      "{'find': 'collection', 'filter': {}, 'comment': 'hi'}";
   test_data.expected_result = "[{'_id': 1}]";
   _test_collection_find (&test_data);
}


static void
test_max (void)
{
   test_collection_find_t test_data = TEST_COLLECTION_FIND_INIT;
   test_data.docs = "[{'_id': 1}]";
   test_data.query_input = "{'$query': {}, '$max': {'_id': 100}}";
   test_data.expected_op_query = test_data.query_input;
   test_data.expected_find_command =
      "{'find': 'collection', 'filter': {}, 'max': {'_id': 100}}";
   test_data.expected_result = "[{'_id': 1}]";
   _test_collection_find (&test_data);
}


static void
test_snapshot (void)
{
   test_collection_find_t test_data = TEST_COLLECTION_FIND_INIT;
   test_data.docs = "[{'_id': 1}]";
   test_data.query_input = "{'$query': {}, '$snapshot': true}";
   test_data.expected_op_query = test_data.query_input;
   test_data.expected_find_command =
      "{'find': 'collection', 'filter': {}, 'snapshot': true}";
   test_data.expected_result = "[{'_id': 1}]";
   _test_collection_find (&test_data);
}


/* $showDiskLoc becomes showRecordId */
static void
test_diskloc (void)
{
   test_collection_find_t test_data = TEST_COLLECTION_FIND_INIT;
   test_data.docs = "[{'_id': 1}]";
   test_data.query_input = "{'$query': {}, '$showDiskLoc': true}";
   test_data.expected_op_query = test_data.query_input;
   test_data.expected_find_command =
      "{'find': 'collection', 'filter': {}, 'showRecordId': true}";
   test_data.expected_result = "[{'_id': 1}]";
   _test_collection_find (&test_data);
}


static void
test_returnkey (void)
{
   test_collection_find_t test_data = TEST_COLLECTION_FIND_INIT;
   test_data.docs = "[{'_id': 1}]";
   test_data.query_input = "{'$query': {}, '$returnKey': true}";
   test_data.expected_op_query = test_data.query_input;
   test_data.expected_find_command =
      "{'find': 'collection', 'filter': {}, 'returnKey': true}";
   test_data.expected_result = "[{}]";
   _test_collection_find (&test_data);
}


static void
test_skip (void)
{
   test_collection_find_t test_data = TEST_COLLECTION_FIND_INIT;
   test_data.docs = "[{'_id': 1}, {'_id': 2}]";
   test_data.skip = 1;
   test_data.query_input = "{'$query': {}, '$orderby': {'_id': 1}}";
   test_data.expected_op_query = test_data.query_input;
   test_data.expected_find_command = "{'find': 'collection', 'filter': {}, "
                                     "'sort': {'_id': 1}, 'skip': "
                                     "{'$numberLong': '1'}}";
   test_data.expected_result = "[{'_id': 2}]";
   _test_collection_find (&test_data);
}


static void
test_batch_size (void)
{
   test_collection_find_t test_data = TEST_COLLECTION_FIND_INIT;
   test_data.docs = "[{'_id': 1}]";
   test_data.batch_size = 2;
   test_data.n_return = 2;
   test_data.expected_find_command =
      "{'find': 'collection', 'filter': {}, 'batchSize': {'$numberLong': '2'}}";
   test_data.expected_result = "[{'_id': 1}]";
   _test_collection_find (&test_data);
}


static void
test_limit (void)
{
   test_collection_find_t test_data = TEST_COLLECTION_FIND_INIT;
   test_data.docs = "[{'_id': 1}, {'_id': 2}, {'_id': 3}]";
   test_data.limit = 2;
   test_data.query_input = "{'$query': {}, '$orderby': {'_id': 1}}";
   test_data.expected_op_query = test_data.query_input;
   test_data.n_return = 2;
   test_data.expected_find_command = "{'find': 'collection', 'filter': {}, "
                                     "'sort': {'_id': 1}, 'limit': "
                                     "{'$numberLong': '2'}}";
   test_data.expected_result = "[{'_id': 1}, {'_id': 2}]";
   _test_collection_find (&test_data);
}


static void
test_negative_limit (void)
{
   test_collection_find_t test_data = TEST_COLLECTION_FIND_INIT;
   test_data.docs = "[{'_id': 1}, {'_id': 2}, {'_id': 3}]";
   test_data.limit = -2;
   test_data.query_input = "{'$query': {}, '$orderby': {'_id': 1}}";
   test_data.expected_op_query = test_data.query_input;
   test_data.n_return = -2;
   test_data.expected_find_command = "{'find': 'collection', 'filter': {}, "
                                     "'sort': {'_id': 1}, 'singleBatch': true, "
                                     "'limit': {'$numberLong': '2'}}";
   test_data.expected_result = "[{'_id': 1}, {'_id': 2}]";
   _test_collection_find (&test_data);
}


static void
test_unrecognized_dollar_option (void)
{
   test_collection_find_t test_data = TEST_COLLECTION_FIND_INIT;

   test_data.query_input = "{'$query': {'a': 1}, '$dumb': 1}";
   test_data.expected_op_query = test_data.query_input;
   test_data.expected_find_command =
      "{'find': 'collection', 'filter': {'a': 1}, 'dumb': 1}";

   test_data.do_live = false;
   _test_collection_find (&test_data);
}


static void
test_query_flags (void)
{
   int i;
   char *find_cmd;
   test_collection_find_t test_data = TEST_COLLECTION_FIND_INIT;

   typedef struct {
      mongoc_query_flags_t flag;
      const char *json_fragment;
   } flag_and_name_t;

   /* slaveok is not supported as an option, exhaust is tested separately */
   flag_and_name_t flags_and_frags[] = {
      {MONGOC_QUERY_TAILABLE_CURSOR, "'tailable': true"},
      {MONGOC_QUERY_OPLOG_REPLAY, "'oplogReplay': true"},
      {MONGOC_QUERY_NO_CURSOR_TIMEOUT, "'noCursorTimeout': true"},
      {MONGOC_QUERY_PARTIAL, "'allowPartialResults': true"},
      {MONGOC_QUERY_TAILABLE_CURSOR | MONGOC_QUERY_AWAIT_DATA,
       "'tailable': true, 'awaitData': true"},
   };

   test_data.expected_result = test_data.docs = "[{'_id': 1}]";

   for (i = 0; i < (sizeof flags_and_frags) / (sizeof (flag_and_name_t)); i++) {
      find_cmd = bson_strdup_printf ("{'find': 'collection', 'filter': {}, %s}",
                                     flags_and_frags[i].json_fragment);

      test_data.flags = flags_and_frags[i].flag;
      test_data.expected_find_command = find_cmd;

      _test_collection_find (&test_data);

      bson_free (find_cmd);
   }
}


static void
test_exhaust (void)
{
   mock_server_t *server;
   mongoc_client_t *client;
   mongoc_collection_t *collection;
   mongoc_cursor_t *cursor;
   request_t *request;
   future_t *future;
   const bson_t *doc;
   bson_error_t error;

   server = mock_server_with_autoismaster (WIRE_VERSION_FIND_CMD);
   mock_server_run (server);
   client = mongoc_client_new_from_uri (mock_server_get_uri (server));
   collection = mongoc_client_get_collection (client, "db", "collection");
   cursor = mongoc_collection_find (
      collection, MONGOC_QUERY_EXHAUST, 0, 0, 0, tmp_bson (NULL), NULL, NULL);

   future = future_cursor_next (cursor, &doc);

   /* Find, getMore and killCursors commands spec: "The find command does not
    * support the exhaust flag from OP_QUERY. Drivers that support exhaust MUST
    * fallback to existing OP_QUERY wire protocol messages."
    */
   request = mock_server_receives_request (server);
   mock_server_replies_to_find (request,
                                MONGOC_QUERY_SLAVE_OK | MONGOC_QUERY_EXHAUST,
                                0,
                                0,
                                "db.collection",
                                "{}",
                                false /* is_command */);

   ASSERT (future_get_bool (future));
   ASSERT_OR_PRINT (!mongoc_cursor_error (cursor, &error), error);

   request_destroy (request);
   future_destroy (future);
   mongoc_cursor_destroy (cursor);
   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
   mock_server_destroy (server);
}


static void
test_getmore_batch_size (void)
{
   mock_server_t *server;
   mongoc_client_t *client;
   mongoc_collection_t *collection;
   mongoc_cursor_t *cursor;
   future_t *future;
   request_t *request;
   const bson_t *doc;
   uint32_t batch_sizes[] = {0, 1, 2};
   size_t i;
   char *batch_size_json;
   bson_error_t error;

   server = mock_server_with_autoismaster (4);
   mock_server_run (server);
   client = mongoc_client_new_from_uri (mock_server_get_uri (server));
   collection = mongoc_client_get_collection (client, "db", "collection");

   for (i = 0; i < sizeof (batch_sizes) / sizeof (uint32_t); i++) {
      cursor = mongoc_collection_find (collection,
                                       MONGOC_QUERY_NONE,
                                       0,
                                       0,
                                       batch_sizes[i],
                                       tmp_bson ("{}"),
                                       NULL,
                                       NULL);

      future = future_cursor_next (cursor, &doc);

      if (batch_sizes[i]) {
         batch_size_json =
            bson_strdup_printf ("{'$numberLong': '%u'}", batch_sizes[i]);
      } else {
         batch_size_json = bson_strdup ("{'$exists': false}");
      }

      request = mock_server_receives_command (
         server,
         "db",
         MONGOC_QUERY_SLAVE_OK,
         "{'find': 'collection', 'filter': {}, 'batchSize': %s}",
         batch_size_json);

      mock_server_replies_simple (request,
                                  "{'ok': 1,"
                                  " 'cursor': {"
                                  "    'id': 0,"
                                  "    'ns': 'db.collection',"
                                  "    'firstBatch': []}}");

      /* no result */
      ASSERT (!future_get_bool (future));
      ASSERT_OR_PRINT (!mongoc_cursor_error (cursor, &error), error);

      future_destroy (future);
      request_destroy (request);
      bson_free (batch_size_json);
      mongoc_cursor_destroy (cursor);
   }

   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
   mock_server_destroy (server);
}


static void
test_getmore_invalid_reply (void *ctx)
{
   mock_server_t *server;
   mongoc_client_t *client;
   mongoc_collection_t *collection;
   mongoc_cursor_t *cursor;
   future_t *future;
   request_t *request;
   const bson_t *doc;
   bson_error_t error;

   if (!TestSuite_CheckMockServerAllowed ()) {
      return;
   }

   server = mock_server_with_autoismaster (4);
   mock_server_run (server);
   client = mongoc_client_new_from_uri (mock_server_get_uri (server));
   collection = mongoc_client_get_collection (client, "db", "collection");

   cursor = mongoc_collection_find (
      collection, MONGOC_QUERY_NONE, 0, 0, 0, tmp_bson ("{}"), NULL, NULL);

   future = future_cursor_next (cursor, &doc);
   request =
      mock_server_receives_command (server,
                                    "db",
                                    MONGOC_QUERY_SLAVE_OK,
                                    "{'find': 'collection', 'filter': {}}");

   mock_server_replies_simple (request,
                               "{'ok': 1,"
                               " 'cursor': {"
                               "    'id': {'$numberLong': '123'},"
                               "    'ns': 'db.collection',"
                               "    'firstBatch': [{}]}}");

   ASSERT (future_get_bool (future));

   future_destroy (future);
   request_destroy (request);

   future = future_cursor_next (cursor, &doc);
   request = mock_server_receives_command (
      server,
      "db",
      MONGOC_QUERY_SLAVE_OK,
      "{'getMore': {'$numberLong': '123'}, 'collection': 'collection'}");

   /* missing "cursor" */
   mock_server_replies_simple (request, "{'ok': 1}");

   ASSERT (!future_get_bool (future));
   ASSERT (mongoc_cursor_error (cursor, &error));
   ASSERT_CMPINT (error.domain, ==, MONGOC_ERROR_PROTOCOL);
   ASSERT_CMPINT (error.code, ==, MONGOC_ERROR_PROTOCOL_INVALID_REPLY);
   ASSERT_CONTAINS (error.message, "getMore");

   future_destroy (future);
   request_destroy (request);
   mongoc_cursor_destroy (cursor);
   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
   mock_server_destroy (server);
}


static void
test_getmore_await (void)
{
   typedef struct {
      mongoc_query_flags_t flags;
      bool expect_await;
   } await_test_t;

   await_test_t await_tests[] = {
      {MONGOC_QUERY_NONE, false},
      {MONGOC_QUERY_TAILABLE_CURSOR, false},
      {MONGOC_QUERY_AWAIT_DATA, false},
      {MONGOC_QUERY_TAILABLE_CURSOR | MONGOC_QUERY_AWAIT_DATA, true},
   };

   mock_server_t *server;
   mongoc_client_t *client;
   mongoc_collection_t *collection;
   mongoc_cursor_t *cursor;
   future_t *future;
   request_t *request;
   const bson_t *doc;
   size_t i;
   char *max_time_json;

   server = mock_server_with_autoismaster (4);
   mock_server_run (server);
   client = mongoc_client_new_from_uri (mock_server_get_uri (server));
   collection = mongoc_client_get_collection (client, "db", "collection");

   for (i = 3; i < sizeof (await_tests) / sizeof (await_test_t); i++) {
      cursor = mongoc_collection_find (collection,
                                       await_tests[i].flags,
                                       0,
                                       0,
                                       0,
                                       tmp_bson ("{}"),
                                       NULL,
                                       NULL);

      ASSERT (mongoc_cursor_is_alive (cursor));

      ASSERT_CMPINT (0, ==, mongoc_cursor_get_max_await_time_ms (cursor));
      mongoc_cursor_set_max_await_time_ms (cursor, 123);
      future = future_cursor_next (cursor, &doc);

      /* only the slave ok bit is still in the query header */
      request = mock_server_receives_command (
         server,
         "db",
         MONGOC_QUERY_SLAVE_OK,
         "{'find': 'collection',"
         " 'maxTimeMS': {'$exists': false},"
         " 'maxAwaitTimeMS': {'$exists': false}}");

      /* reply with cursor id 1 */
      mock_server_replies_simple (request,
                                  "{'ok': 1,"
                                  " 'cursor': {"
                                  "    'id': 1,"
                                  "    'ns': 'db.collection',"
                                  "    'firstBatch': [{}]}}");

      /* no result or error */
      ASSERT (future_get_bool (future));
      ASSERT (mongoc_cursor_is_alive (cursor));

      future_destroy (future);
      request_destroy (request);

      future = future_cursor_next (cursor, &doc);

      if (await_tests[i].expect_await) {
         max_time_json = "123";
      } else {
         max_time_json = "{'$exists': false}";
      }

      /* only the slave ok bit is still in the query header */
      request =
         mock_server_receives_command (server,
                                       "db",
                                       MONGOC_QUERY_SLAVE_OK,
                                       "{'getMore': {'$numberLong': '1'},"
                                       " 'collection': 'collection',"
                                       " 'maxAwaitTimeMS': {'$exists': false},"
                                       " 'maxTimeMS': %s}",
                                       max_time_json);

      BSON_ASSERT (request);
      /* reply with cursor id 0 */
      mock_server_replies_simple (request,
                                  "{'ok': 1,"
                                  " 'cursor': {"
                                  "    'id': 0,"
                                  "    'ns': 'db.collection',"
                                  "    'nextBatch': []}}");

      /* no result or error */
      ASSERT (!future_get_bool (future));
      ASSERT (!mongoc_cursor_error (cursor, NULL));
      ASSERT (!mongoc_cursor_is_alive (cursor));
      ASSERT (!doc);

      future_destroy (future);
      request_destroy (request);
      mongoc_cursor_destroy (cursor);
   }

   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
   mock_server_destroy (server);
}


static void
_test_tailable_timeout (bool pooled)
{
   mongoc_client_pool_t *pool = NULL;
   mongoc_client_t *client;
   mongoc_database_t *database;
   char *collection_name;
   mongoc_collection_t *collection;
   bool r;
   bson_error_t error;
   mongoc_cursor_t *cursor;
   const bson_t *doc;
   bson_t reply;

   capture_logs (true);

   if (pooled) {
      pool = test_framework_client_pool_new ();
      client = mongoc_client_pool_pop (pool);
   } else {
      client = test_framework_client_new ();
   }

   database = mongoc_client_get_database (client, "test");
   collection_name = gen_collection_name ("test");

   collection = mongoc_database_get_collection (database, collection_name);
   mongoc_collection_drop (collection, NULL);
   mongoc_collection_destroy (collection);

   collection = mongoc_database_create_collection (
      database,
      collection_name,
      tmp_bson ("{'capped': true, 'size': 10000}"),
      &error);

   ASSERT_OR_PRINT (collection, error);

   r = mongoc_collection_insert (
      collection, MONGOC_INSERT_NONE, tmp_bson ("{}"), NULL, &error);

   ASSERT_OR_PRINT (r, error);

   client->cluster.sockettimeoutms = 100;
   cursor = mongoc_collection_find (collection,
                                    MONGOC_QUERY_TAILABLE_CURSOR |
                                       MONGOC_QUERY_AWAIT_DATA,
                                    0,
                                    0,
                                    0,
                                    tmp_bson ("{'a': 1}"),
                                    NULL,
                                    NULL);

   ASSERT (!mongoc_cursor_next (cursor, &doc));

   client->cluster.sockettimeoutms = 30 * 1000 * 1000;
   r = mongoc_client_command_simple (
      client, "test", tmp_bson ("{'buildinfo': 1}"), NULL, &reply, &error);

   ASSERT_OR_PRINT (r, error);
   ASSERT_HAS_FIELD (&reply, "version");

   bson_destroy (&reply);
   mongoc_cursor_destroy (cursor);
   mongoc_collection_destroy (collection);
   mongoc_database_destroy (database);
   bson_free (collection_name);

   if (pooled) {
      mongoc_client_pool_push (pool, client);
      mongoc_client_pool_destroy (pool);
   } else {
      mongoc_client_destroy (client);
   }
}


static void
test_tailable_timeout_single (void)
{
   _test_tailable_timeout (false);
}


#ifndef MONGOC_ENABLE_SSL_SECURE_TRANSPORT
static void
test_tailable_timeout_pooled (void)
{
   _test_tailable_timeout (true);
}
#endif


void
test_collection_find_install (TestSuite *suite)
{
   TestSuite_AddLive (
      suite, "/Collection/find/dollar_query", test_dollar_query);
   TestSuite_AddLive (suite, "/Collection/find/dollar_or", test_dollar_or);
   TestSuite_AddLive (suite,
                      "/Collection/find/mixed_dollar_nondollar",
                      test_mixed_dollar_nondollar);
   TestSuite_AddLive (
      suite, "/Collection/find/key_named_filter", test_key_named_filter);
   TestSuite_AddLive (suite,
                      "/Collection/find/key_named_filter/$query",
                      test_key_named_filter_with_dollar_query);
   TestSuite_AddLive (
      suite, "/Collection/find/subdoc_named_filter", test_subdoc_named_filter);
   TestSuite_AddLive (suite,
                      "/Collection/find/subdoc_named_filter/$query",
                      test_subdoc_named_filter_with_dollar_query);
   TestSuite_AddLive (suite, "/Collection/find/newoption", test_newoption);
   TestSuite_AddLive (suite, "/Collection/find/orderby", test_orderby);
   TestSuite_AddLive (suite, "/Collection/find/fields", test_fields);
   TestSuite_AddLive (
      suite, "/Collection/find/modifiers/integer", test_int_modifiers);
   TestSuite_AddLive (suite,
                      "/Collection/find/modifiers/index_spec",
                      test_index_spec_modifiers);
   TestSuite_AddLive (suite, "/Collection/find/comment", test_comment);
   TestSuite_AddLive (suite, "/Collection/find/max", test_max);
   TestSuite_AddLive (suite, "/Collection/find/modifiers/bool", test_snapshot);
   TestSuite_AddLive (suite, "/Collection/find/showdiskloc", test_diskloc);
   TestSuite_AddLive (suite, "/Collection/find/returnkey", test_returnkey);
   TestSuite_AddLive (suite, "/Collection/find/skip", test_skip);
   TestSuite_AddLive (suite, "/Collection/find/batch_size", test_batch_size);
   TestSuite_AddLive (suite, "/Collection/find/limit", test_limit);
   TestSuite_AddLive (
      suite, "/Collection/find/negative_limit", test_negative_limit);
   TestSuite_Add (
      suite, "/Collection/find/unrecognized", test_unrecognized_dollar_option);
   TestSuite_AddLive (suite, "/Collection/find/flags", test_query_flags);
   TestSuite_AddMockServerTest (
      suite, "/Collection/find/exhaust", test_exhaust);
   TestSuite_AddMockServerTest (
      suite, "/Collection/getmore/batch_size", test_getmore_batch_size);
   TestSuite_AddFull (suite,
                      "/Collection/getmore/invalid_reply",
                      test_getmore_invalid_reply,
                      NULL,
                      NULL,
                      test_framework_skip_if_slow);
   TestSuite_AddMockServerTest (
      suite, "/Collection/getmore/await", test_getmore_await);
   TestSuite_AddLive (suite,
                      "/Collection/tailable/timeout/single",
                      test_tailable_timeout_single);
#ifndef MONGOC_ENABLE_SSL_SECURE_TRANSPORT
#ifndef MONGOC_ENABLE_SSL_SECURE_CHANNEL
   TestSuite_AddLive (suite,
                      "/Collection/tailable/timeout/pooled",
                      test_tailable_timeout_pooled);
#endif
#endif
}
