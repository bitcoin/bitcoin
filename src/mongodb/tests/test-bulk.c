#include <mongoc.h>
#include <mongoc-bulk-operation-private.h>
#include <mongoc-client-private.h>
#include <mongoc-cursor-private.h>
#include <mongoc-collection-private.h>
#include <mongoc-util-private.h>

#include "TestSuite.h"

#include "test-libmongoc.h"
#include "mock_server/future-functions.h"
#include "mock_server/mock-server.h"
#include "test-conveniences.h"
#include "mock_server/mock-rs.h"


/*--------------------------------------------------------------------------
 *
 * server_has_write_commands --
 *
 *       Decide with wire version if server supports write commands
 *
 * Returns:
 *       True or false.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

static bool
server_has_write_commands ()
{
   return test_framework_max_wire_version_at_least (1);
}


/*--------------------------------------------------------------------------
 *
 * check_n_modified --
 *
 *       Check a bulk operation reply's nModified field is correct or absent.
 *
 *       It may be omitted if we talked to a (<= 2.4.x) node, or a mongos
 *       talked to a (<= 2.4.x) node.
 *
 * Returns:
 *       None.
 *
 * Side effects:
 *       Aborts if the field is incorrect.
 *
 *--------------------------------------------------------------------------
 */

static void
check_n_modified (bool has_write_commands,
                  const bson_t *reply,
                  int32_t n_modified)
{
   bson_iter_t iter;

   if (bson_iter_init_find (&iter, reply, "nModified")) {
      BSON_ASSERT (has_write_commands);
      BSON_ASSERT (BSON_ITER_HOLDS_INT32 (&iter));
      BSON_ASSERT (bson_iter_int32 (&iter) == n_modified);
   } else {
      BSON_ASSERT (!has_write_commands);
   }
}


/*--------------------------------------------------------------------------
 *
 * assert_error_count --
 *
 *       Check the length of a bulk operation reply's writeErrors.
 *
 * Returns:
 *       None.
 *
 * Side effects:
 *       Aborts if the array is the wrong length.
 *
 *--------------------------------------------------------------------------
 */

static void
assert_error_count (int len, const bson_t *reply)
{
   bson_iter_t iter;
   bson_iter_t error_iter;
   int n = 0;

   BSON_ASSERT (bson_iter_init_find (&iter, reply, "writeErrors"));
   BSON_ASSERT (bson_iter_recurse (&iter, &error_iter));
   while (bson_iter_next (&error_iter)) {
      n++;
   }
   ASSERT_CMPINT (len, ==, n);
}


/*--------------------------------------------------------------------------
 *
 * assert_n_inserted --
 *
 *       Check a bulk operation reply's nInserted field.
 *
 * Returns:
 *       None.
 *
 * Side effects:
 *       Aborts if the field is incorrect.
 *
 *--------------------------------------------------------------------------
 */

static void
assert_n_inserted (int n, const bson_t *reply)
{
   bson_iter_t iter;

   BSON_ASSERT (bson_iter_init_find (&iter, reply, "nInserted"));
   BSON_ASSERT (BSON_ITER_HOLDS_INT32 (&iter));
   ASSERT_CMPINT (n, ==, bson_iter_int32 (&iter));
}


/*--------------------------------------------------------------------------
 *
 * assert_n_removed --
 *
 *       Check a bulk operation reply's nRemoved field.
 *
 * Returns:
 *       None.
 *
 * Side effects:
 *       Aborts if the field is incorrect.
 *
 *--------------------------------------------------------------------------
 */

static void
assert_n_removed (int n, const bson_t *reply)
{
   bson_iter_t iter;

   BSON_ASSERT (bson_iter_init_find (&iter, reply, "nRemoved"));
   BSON_ASSERT (BSON_ITER_HOLDS_INT32 (&iter));
   ASSERT_CMPINT (n, ==, bson_iter_int32 (&iter));
}


/*--------------------------------------------------------------------------
 *
 * oid_created_on_client --
 *
 *       Check that a document's _id contains this process's pid.
 *
 * Returns:
 *       True or false.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

static bool
oid_created_on_client (const bson_t *doc)
{
   bson_oid_t new_oid;
   const uint8_t *new_pid;
   bson_iter_t iter;
   const bson_oid_t *oid;
   const uint8_t *pid;

   bson_oid_init (&new_oid, NULL);
   new_pid = &new_oid.bytes[7];

   bson_iter_init_find (&iter, doc, "_id");

   if (!BSON_ITER_HOLDS_OID (&iter)) {
      return false;
   }

   oid = bson_iter_oid (&iter);
   pid = &oid->bytes[7];

   return 0 == memcmp (pid, new_pid, 2);
}


static void
create_unique_index (mongoc_collection_t *collection)
{
   mongoc_index_opt_t opt;
   bson_error_t error;

   mongoc_index_opt_init (&opt);
   opt.unique = true;

   BEGIN_IGNORE_DEPRECATIONS
   ASSERT_OR_PRINT (mongoc_collection_create_index (
                       collection, tmp_bson ("{'a': 1}"), &opt, &error),
                    error);
   END_IGNORE_DEPRECATIONS
}


static void
test_bulk (void)
{
   mongoc_bulk_operation_t *bulk;
   mongoc_collection_t *collection;
   mongoc_client_t *client;
   bool has_write_cmds;
   bson_error_t error;
   bson_t reply;
   bson_t child;
   bson_t del;
   bson_t up;
   bson_t doc = BSON_INITIALIZER;

   client = test_framework_client_new ();
   BSON_ASSERT (client);
   has_write_cmds = server_has_write_commands ();

   collection = get_test_collection (client, "test_bulk");
   BSON_ASSERT (collection);

   bulk = mongoc_collection_create_bulk_operation (collection, true, NULL);
   BSON_ASSERT (bulk);

   mongoc_bulk_operation_insert (bulk, &doc);
   mongoc_bulk_operation_insert (bulk, &doc);
   mongoc_bulk_operation_insert (bulk, &doc);
   mongoc_bulk_operation_insert (bulk, &doc);

   bson_init (&up);
   bson_append_document_begin (&up, "$set", -1, &child);
   bson_append_int32 (&child, "hello", -1, 123);
   bson_append_document_end (&up, &child);
   mongoc_bulk_operation_update (bulk, &doc, &up, false);
   bson_destroy (&up);

   bson_init (&del);
   BSON_APPEND_INT32 (&del, "hello", 123);
   mongoc_bulk_operation_remove (bulk, &del);
   bson_destroy (&del);

   ASSERT_OR_PRINT (mongoc_bulk_operation_execute (bulk, &reply, &error),
                    error);

   ASSERT_MATCH (&reply,
                 "{'nInserted': 4,"
                 " 'nMatched':  4,"
                 " 'nRemoved':  4,"
                 " 'nUpserted': 0,"
                 " 'writeErrors': []}");

   check_n_modified (has_write_cmds, &reply, 4);
   ASSERT_COUNT (0, collection);

   bson_destroy (&reply);

   ASSERT_OR_PRINT (mongoc_collection_drop (collection, &error), error);

   mongoc_bulk_operation_destroy (bulk);
   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
}


static void
test_bulk_error (void)
{
   bson_t reply = {0};
   bson_error_t error;
   mongoc_bulk_operation_t *bulk;
   mock_server_t *mock_server;
   mongoc_client_t *client;

   mock_server = mock_server_with_autoismaster (WIRE_VERSION_WRITE_CMD);
   mock_server_run (mock_server);
   client = mongoc_client_new_from_uri (mock_server_get_uri (mock_server));

   bulk = mongoc_bulk_operation_new (true);
   mongoc_bulk_operation_set_client (bulk, client);
   BSON_ASSERT (!mongoc_bulk_operation_execute (bulk, &reply, &error));
   ASSERT_CMPINT (error.domain, ==, MONGOC_ERROR_COMMAND);
   ASSERT_CMPINT (error.code, ==, MONGOC_ERROR_COMMAND_INVALID_ARG);

   /* reply was initialized */
   ASSERT_CMPUINT32 (reply.len, ==, (uint32_t) 5);
   mongoc_bulk_operation_destroy (bulk);
   mongoc_client_destroy (client);
   mock_server_destroy (mock_server);
}


static void
test_bulk_error_unordered (void)
{
   mock_server_t *mock_server;
   mongoc_client_t *client;
   mongoc_collection_t *collection;
   mongoc_bulk_operation_t *bulk;
   bson_t reply;
   bson_error_t error;
   request_t *request;
   future_t *future;
   int i;
   mongoc_uri_t *uri;

   mock_server = mock_server_with_autoismaster (WIRE_VERSION_WRITE_CMD);
   mock_server_run (mock_server);

   uri = mongoc_uri_copy (mock_server_get_uri (mock_server));
   mongoc_uri_set_option_as_int32 (uri, "sockettimeoutms", 500);
   client = mongoc_client_new_from_uri (uri);
   mongoc_uri_destroy (uri);
   collection = mongoc_client_get_collection (client, "test", "test");

   bulk = mongoc_collection_create_bulk_operation (collection, false, NULL);
   for (i = 0; i <= 2048; i++) {
      mongoc_bulk_operation_update_many_with_opts (
         bulk,
         tmp_bson ("{'hello': 'earth'}"),
         tmp_bson ("{'$set': {'hello': 'world'}}"),
         NULL,
         &error);
   }

   future = future_bulk_operation_execute (bulk, &reply, &error);

   request = mock_server_receives_command (
      mock_server,
      "test",
      MONGOC_QUERY_NONE,
      "{ 'update' : 'test', 'writeConcern' : {  }, 'ordered' : false }",
      NULL);
   mock_server_replies_simple (request, "{ 'ok' : 1, 'n' : 5 }");

   request_destroy (request);
   request = mock_server_receives_command (
      mock_server,
      "test",
      MONGOC_QUERY_NONE,
      "{ 'update' : 'test', 'writeConcern' : {  }, 'ordered' : false }",
      NULL);

   request_destroy (request);
   mock_server_destroy (mock_server);

   future_wait_max (future, 100);
   ASSERT (!future_value_get_uint32_t (&future->return_value));
   future_destroy (future);
   ASSERT_ERROR_CONTAINS (error,
                          MONGOC_ERROR_STREAM,
                          MONGOC_ERROR_STREAM_SOCKET,
                          "socket error or timeout");

   ASSERT_MATCH (&reply,
                 "{'nInserted': 0,"
                 " 'nMatched':  5,"
                 " 'nRemoved':  0,"
                 " 'nUpserted': 0}");

   bson_destroy (&reply);
   mongoc_bulk_operation_destroy (bulk);
   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
}

static void
test_insert (bool ordered)
{
   mongoc_bulk_operation_t *bulk;
   mongoc_collection_t *collection;
   mongoc_client_t *client;
   bool has_write_cmds;
   bson_error_t error;
   bson_t reply;
   bson_t doc = BSON_INITIALIZER;
   bson_t query = BSON_INITIALIZER;
   mongoc_cursor_t *cursor;
   const bson_t *inserted_doc;

   client = test_framework_client_new ();
   BSON_ASSERT (client);
   has_write_cmds = server_has_write_commands ();

   collection = get_test_collection (client, "test_insert");
   BSON_ASSERT (collection);

   bulk = mongoc_collection_create_bulk_operation (collection, ordered, NULL);
   BSON_ASSERT (bulk);
   BSON_ASSERT (bulk->flags.ordered == ordered);

   mongoc_bulk_operation_insert (bulk, &doc);
   mongoc_bulk_operation_insert (bulk, &doc);

   ASSERT_OR_PRINT (mongoc_bulk_operation_execute (bulk, &reply, &error),
                    error);

   ASSERT_MATCH (&reply,
                 "{'nInserted': 2,"
                 " 'nMatched':  0,"
                 " 'nRemoved':  0,"
                 " 'nUpserted': 0}");

   check_n_modified (has_write_cmds, &reply, 0);

   bson_destroy (&reply);
   ASSERT_COUNT (2, collection);

   cursor = mongoc_collection_find_with_opts (collection, &query, NULL, NULL);
   BSON_ASSERT (cursor);

   while (mongoc_cursor_next (cursor, &inserted_doc)) {
      BSON_ASSERT (oid_created_on_client (inserted_doc));
   }

   ASSERT_OR_PRINT (mongoc_collection_drop (collection, &error), error);

   mongoc_cursor_destroy (cursor);
   bson_destroy (&query);
   mongoc_bulk_operation_destroy (bulk);
   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
   bson_destroy (&doc);
}


static void
test_insert_ordered (void)
{
   test_insert (true);
}


static void
test_insert_unordered (void)
{
   test_insert (false);
}


static void
test_insert_check_keys (void)
{
   mongoc_bulk_operation_t *bulk;
   mongoc_collection_t *collection;
   mongoc_client_t *client;
   bson_t reply;
   bson_error_t error;
   bool r;

   capture_logs (true);

   client = test_framework_client_new ();
   BSON_ASSERT (client);
   collection = get_test_collection (client, "test_insert_check_keys");
   BSON_ASSERT (collection);

   /* keys can't start with "$" */
   bulk = mongoc_collection_create_bulk_operation (collection, true, NULL);
   BSON_ASSERT (bulk);

   mongoc_bulk_operation_insert (bulk, tmp_bson ("{'$dollar': 1}"));
   r = (bool) mongoc_bulk_operation_execute (bulk, &reply, &error);
   BSON_ASSERT (!r);
   ASSERT_ERROR_CONTAINS (error,
                          MONGOC_ERROR_COMMAND,
                          MONGOC_ERROR_COMMAND_INVALID_ARG,
                          "document to insert contains invalid key: keys "
                          "cannot begin with \"$\": \"$dollar\"");

   BSON_ASSERT (bson_empty (&reply));

   bson_destroy (&reply);
   mongoc_bulk_operation_destroy (bulk);

   /* valid, then invalid */
   bulk = mongoc_collection_create_bulk_operation (collection, true, NULL);
   BSON_ASSERT (bulk);

   mongoc_bulk_operation_insert (bulk, tmp_bson (NULL));
   mongoc_bulk_operation_insert (bulk, tmp_bson ("{'$dollar': 1}"));
   r = (bool) mongoc_bulk_operation_execute (bulk, &reply, &error);
   BSON_ASSERT (!r);
   ASSERT_ERROR_CONTAINS (error,
                          MONGOC_ERROR_COMMAND,
                          MONGOC_ERROR_COMMAND_INVALID_ARG,
                          "document to insert contains invalid key: keys "
                          "cannot begin with \"$\": \"$dollar\"");

   BSON_ASSERT (bson_empty (&reply));

   bson_destroy (&reply);
   mongoc_bulk_operation_destroy (bulk);

   /* keys can't contain "." */
   bulk = mongoc_collection_create_bulk_operation (collection, true, NULL);
   BSON_ASSERT (bulk);

   mongoc_bulk_operation_insert (bulk, tmp_bson ("{'a.b': 1}"));
   r = (bool) mongoc_bulk_operation_execute (bulk, &reply, &error);
   BSON_ASSERT (!r);
   ASSERT_ERROR_CONTAINS (error,
                          MONGOC_ERROR_COMMAND,
                          MONGOC_ERROR_COMMAND_INVALID_ARG,
                          "document to insert contains invalid key: keys "
                          "cannot contain \".\": \"a.b\"");

   BSON_ASSERT (bson_empty (&reply));

   bson_destroy (&reply);
   mongoc_bulk_operation_destroy (bulk);
   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
}


static void
test_upsert (bool ordered)
{
   mongoc_bulk_operation_t *bulk;
   mongoc_collection_t *collection;
   mongoc_client_t *client;
   bool has_write_cmds;

   bson_error_t error;
   bson_t reply;
   bson_t *sel;
   bson_t *doc;

   client = test_framework_client_new ();
   BSON_ASSERT (client);
   has_write_cmds = server_has_write_commands ();

   collection = get_test_collection (client, "test_upsert");
   BSON_ASSERT (collection);

   bulk = mongoc_collection_create_bulk_operation (collection, ordered, NULL);
   BSON_ASSERT (bulk);

   sel = tmp_bson ("{'_id': 1234}");
   doc = tmp_bson ("{'$set': {'hello': 'there'}}");

   mongoc_bulk_operation_update (bulk, sel, doc, true);

   ASSERT_OR_PRINT (mongoc_bulk_operation_execute (bulk, &reply, &error),
                    error);

   ASSERT_MATCH (&reply,
                 "{'nInserted': 0,"
                 " 'nMatched':  0,"
                 " 'nRemoved':  0,"
                 " 'nUpserted': 1,"
                 " 'upserted':  [{'index': 0, '_id': 1234}],"
                 " 'writeErrors': []}");

   check_n_modified (has_write_cmds, &reply, 0);
   ASSERT_COUNT (1, collection);

   bson_destroy (&reply);
   mongoc_bulk_operation_destroy (bulk);

   bulk = mongoc_collection_create_bulk_operation (collection, ordered, NULL);
   BSON_ASSERT (bulk);

   /* non-upsert, no matches */
   sel = tmp_bson ("{'_id': 2}");
   doc = tmp_bson ("{'$set': {'hello': 'there'}}");

   mongoc_bulk_operation_update (bulk, sel, doc, false);
   ASSERT_OR_PRINT (mongoc_bulk_operation_execute (bulk, &reply, &error),
                    error);

   ASSERT_MATCH (&reply,
                 "{'nInserted': 0,"
                 " 'nMatched':  0,"
                 " 'nRemoved':  0,"
                 " 'nUpserted': 0,"
                 " 'upserted':  {'$exists': false},"
                 " 'writeErrors': []}");

   check_n_modified (has_write_cmds, &reply, 0);
   ASSERT_COUNT (1, collection); /* doc remains from previous operation */

   ASSERT_OR_PRINT (mongoc_collection_drop (collection, &error), error);

   bson_destroy (&reply);
   mongoc_bulk_operation_destroy (bulk);
   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
}


static void
test_upsert_ordered (void)
{
   test_upsert (true);
}


static void
test_upsert_unordered (void)
{
   test_upsert (false);
}


static void
test_upsert_unordered_oversized (void *ctx)
{
   mongoc_client_t *client;
   mongoc_collection_t *collection;
   mongoc_bulk_operation_t *bulk;
   bson_t *u;
   bool r;
   bson_error_t error;
   bson_t reply;

   client = test_framework_client_new ();
   collection = get_test_collection (client, "upsert_oversized");
   bulk = mongoc_collection_create_bulk_operation (
      collection, false /* ordered */, NULL);

   /* much too large */
   u = tmp_bson ("{'$set': {'x': '%s', 'y': '%s'}}",
                 huge_string (client),
                 huge_string (client));

   r = mongoc_bulk_operation_update_one_with_opts (
      bulk, tmp_bson (NULL), u, tmp_bson ("{'upsert': true}"), &error);

   ASSERT_OR_PRINT (r, error);
   r = (bool) mongoc_bulk_operation_execute (bulk, &reply, &error);
   ASSERT (!r);
   ASSERT_ERROR_CONTAINS (error,
                          MONGOC_ERROR_BSON,
                          MONGOC_ERROR_BSON_INVALID,
                          "Document 0 is too large");

   ASSERT_MATCH (&reply,
                 "{'nInserted': 0,"
                 " 'nMatched':  0,"
                 " 'nRemoved':  0,"
                 " 'nUpserted': 0,"
                 " 'writeErrors': []}");

   bson_destroy (&reply);
   mongoc_bulk_operation_destroy (bulk);
   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
}


static void
test_upserted_index (bool ordered)
{
   mongoc_bulk_operation_t *bulk;
   mongoc_collection_t *collection;
   mongoc_client_t *client;
   bool has_write_cmds;

   bson_error_t error;
   bson_t reply;
   bson_t *emp = tmp_bson ("{}");
   bson_t *inc = tmp_bson ("{'$inc': {'b': 1}}");
   bool r;

   client = test_framework_client_new ();
   BSON_ASSERT (client);
   has_write_cmds = server_has_write_commands ();

   collection = get_test_collection (client, "test_upserted_index");
   BSON_ASSERT (collection);

   bulk = mongoc_collection_create_bulk_operation (collection, ordered, NULL);
   BSON_ASSERT (bulk);

   mongoc_bulk_operation_insert (bulk, emp);
   mongoc_bulk_operation_insert (bulk, emp);
   mongoc_bulk_operation_remove (bulk, tmp_bson ("{'i': 2}"));
   mongoc_bulk_operation_update (bulk, tmp_bson ("{'i': 3}"), inc, false);
   /* upsert */
   mongoc_bulk_operation_update (bulk, tmp_bson ("{'i': 4}"), inc, true);
   mongoc_bulk_operation_remove (bulk, tmp_bson ("{'i': 5}"));
   mongoc_bulk_operation_remove_one (bulk, tmp_bson ("{'i': 6}"));
   mongoc_bulk_operation_replace_one (bulk, tmp_bson ("{'i': 7}"), emp, false);
   /* upsert */
   mongoc_bulk_operation_replace_one (bulk, tmp_bson ("{'i': 8}"), emp, true);
   /* upsert */
   mongoc_bulk_operation_replace_one (bulk, tmp_bson ("{'i': 9}"), emp, true);
   mongoc_bulk_operation_remove (bulk, tmp_bson ("{'i': 10}"));
   mongoc_bulk_operation_insert (bulk, emp);
   mongoc_bulk_operation_insert (bulk, emp);
   mongoc_bulk_operation_update (bulk, tmp_bson ("{'i': 13}"), inc, false);
   /* upsert */
   mongoc_bulk_operation_update (bulk, tmp_bson ("{'i': 14}"), inc, true);
   mongoc_bulk_operation_insert (bulk, emp);
   /* upserts */
   mongoc_bulk_operation_update (bulk, tmp_bson ("{'i': 16}"), inc, true);
   mongoc_bulk_operation_update (bulk, tmp_bson ("{'i': 17}"), inc, true);
   /* non-upsert */
   mongoc_bulk_operation_update (bulk, tmp_bson ("{'i': 18}"), inc, false);
   /* upserts */
   mongoc_bulk_operation_update (bulk, tmp_bson ("{'i': 19}"), inc, true);
   mongoc_bulk_operation_replace_one (bulk, tmp_bson ("{'i': 20}"), emp, true);
   mongoc_bulk_operation_replace_one (bulk, tmp_bson ("{'i': 21}"), emp, true);
   mongoc_bulk_operation_replace_one (bulk, tmp_bson ("{'i': 22}"), emp, true);
   mongoc_bulk_operation_update (bulk, tmp_bson ("{'i': 23}"), inc, true);
   /* non-upsert */
   mongoc_bulk_operation_update_one (bulk, tmp_bson ("{'i': 24}"), inc, false);
   /* upsert */
   mongoc_bulk_operation_update_one (bulk, tmp_bson ("{'i': 25}"), inc, true);
   /* non-upserts */
   mongoc_bulk_operation_remove (bulk, tmp_bson ("{'i': 26}"));
   mongoc_bulk_operation_remove (bulk, tmp_bson ("{'i': 27}"));
   mongoc_bulk_operation_update_one (bulk, tmp_bson ("{'i': 28}"), inc, false);
   mongoc_bulk_operation_update_one (bulk, tmp_bson ("{'i': 29}"), inc, false);
   /* each update modifies existing 16 docs, but only increments index by one */
   mongoc_bulk_operation_update (bulk, emp, inc, false);
   mongoc_bulk_operation_update (bulk, emp, inc, false);
   /* upsert */
   mongoc_bulk_operation_update_one (bulk, tmp_bson ("{'i': 32}"), inc, true);


   r = (bool) mongoc_bulk_operation_execute (bulk, &reply, &error);
   if (!r) {
      fprintf (stderr, "bulk failed: %s\n", error.message);
      abort ();
   }

   ASSERT_MATCH (&reply,
                 "{'nInserted':    5,"
                 " 'nMatched':    34,"
                 " 'nRemoved':     0,"
                 " 'nUpserted':   13,"
                 " 'upserted': ["
                 "    {'index':   4},"
                 "    {'index':   8},"
                 "    {'index':   9},"
                 "    {'index':  14},"
                 "    {'index':  16},"
                 "    {'index':  17},"
                 "    {'index':  19},"
                 "    {'index':  20},"
                 "    {'index':  21},"
                 "    {'index':  22},"
                 "    {'index':  23},"
                 "    {'index':  25},"
                 "    {'index':  32}"
                 " ],"
                 " 'writeErrors': []}");

   check_n_modified (has_write_cmds, &reply, 34);
   ASSERT_COUNT (18, collection);

   ASSERT_OR_PRINT (mongoc_collection_drop (collection, &error), error);

   bson_destroy (&reply);
   mongoc_bulk_operation_destroy (bulk);
   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
}


static void
test_upserted_index_ordered (void)
{
   test_upserted_index (true);
}


static void
test_upserted_index_unordered (void)
{
   test_upserted_index (false);
}


static void
test_update_one (bool ordered)
{
   mongoc_bulk_operation_t *bulk;
   mongoc_collection_t *collection;
   mongoc_client_t *client;
   bool has_write_cmds;

   bson_error_t error;
   bson_t reply;
   bson_t *sel;
   bson_t *doc;
   bool r;

   client = test_framework_client_new ();
   BSON_ASSERT (client);
   has_write_cmds = server_has_write_commands ();

   collection = get_test_collection (client, "test_update_one");
   BSON_ASSERT (collection);

   doc = bson_new ();
   r = mongoc_collection_insert (
      collection, MONGOC_INSERT_NONE, doc, NULL, NULL);
   BSON_ASSERT (r);
   r = mongoc_collection_insert (
      collection, MONGOC_INSERT_NONE, doc, NULL, NULL);
   BSON_ASSERT (r);
   bson_destroy (doc);

   bulk = mongoc_collection_create_bulk_operation (collection, ordered, NULL);
   BSON_ASSERT (bulk);

   sel = tmp_bson ("{}");
   doc = tmp_bson ("{'$set': {'hello': 'there'}}");
   mongoc_bulk_operation_update_one (bulk, sel, doc, true);
   ASSERT_OR_PRINT ((bool) mongoc_bulk_operation_execute (bulk, &reply, &error),
                    error);

   ASSERT_MATCH (&reply,
                 "{'nInserted': 0,"
                 " 'nMatched':  1,"
                 " 'nRemoved':  0,"
                 " 'nUpserted': 0,"
                 " 'upserted': {'$exists': false},"
                 " 'writeErrors': []}");

   check_n_modified (has_write_cmds, &reply, 1);
   ASSERT_COUNT (2, collection);

   ASSERT_OR_PRINT (mongoc_collection_drop (collection, &error), error);

   bson_destroy (&reply);
   mongoc_bulk_operation_destroy (bulk);
   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
}


static void
test_update_one_ordered (void)
{
   test_update_one (true);
}


static void
test_update_one_unordered (void)
{
   test_update_one (false);
}


static void
test_replace_one (bool ordered)
{
   mongoc_bulk_operation_t *bulk;
   mongoc_collection_t *collection;
   mongoc_client_t *client;
   bool has_write_cmds;

   bson_error_t error;
   bson_t reply;
   bson_t *sel;
   bson_t *doc;
   bool r;

   client = test_framework_client_new ();
   BSON_ASSERT (client);
   has_write_cmds = server_has_write_commands ();

   collection = get_test_collection (client, "test_replace_one");
   BSON_ASSERT (collection);

   doc = bson_new ();
   r = mongoc_collection_insert (
      collection, MONGOC_INSERT_NONE, doc, NULL, NULL);
   BSON_ASSERT (r);
   r = mongoc_collection_insert (
      collection, MONGOC_INSERT_NONE, doc, NULL, NULL);
   BSON_ASSERT (r);
   bson_destroy (doc);

   bulk = mongoc_collection_create_bulk_operation (collection, ordered, NULL);
   BSON_ASSERT (bulk);

   sel = tmp_bson ("{}");
   doc = tmp_bson ("{'hello': 'there'}");
   mongoc_bulk_operation_replace_one (bulk, sel, doc, true);
   ASSERT_OR_PRINT ((bool) mongoc_bulk_operation_execute (bulk, &reply, &error),
                    error);

   ASSERT_MATCH (&reply,
                 "{'nInserted': 0,"
                 " 'nMatched':  1,"
                 " 'nRemoved':  0,"
                 " 'nUpserted': 0,"
                 " 'upserted': {'$exists': false},"
                 " 'writeErrors': []}");

   check_n_modified (has_write_cmds, &reply, 1);
   ASSERT_COUNT (2, collection);

   ASSERT_OR_PRINT (mongoc_collection_drop (collection, &error), error);

   bson_destroy (&reply);
   mongoc_bulk_operation_destroy (bulk);
   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
}


static void
_test_replace_one_check_keys (bool with_opts)
{
   mongoc_bulk_operation_t *bulk;
   mongoc_collection_t *collection;
   mongoc_client_t *client;

   bson_error_t error;
   bson_t reply;
   bool r;

   client = test_framework_client_new ();
   collection = get_test_collection (client, "test_replace_one_check_keys");
   bulk = mongoc_collection_create_bulk_operation (collection, true, NULL);

   if (with_opts) {
      /* rejected immediately */
      r = mongoc_bulk_operation_replace_one_with_opts (
         bulk, tmp_bson ("{}"), tmp_bson ("{'$a': 1}"), NULL, &error);

      ASSERT (!r);
      ASSERT_ERROR_CONTAINS (error,
                             MONGOC_ERROR_COMMAND,
                             MONGOC_ERROR_COMMAND_INVALID_ARG,
                             "replacement document contains invalid key: keys "
                             "cannot begin with \"$\": \"$a\"");

      r = (bool) mongoc_bulk_operation_execute (bulk, &reply, &error);
      ASSERT (!r);
      ASSERT_ERROR_CONTAINS (error,
                             MONGOC_ERROR_COMMAND,
                             MONGOC_ERROR_COMMAND_INVALID_ARG,
                             "empty bulk write");
   } else {
      /* rejected during execute() */
      capture_logs (true);
      mongoc_bulk_operation_replace_one (
         bulk, tmp_bson ("{}"), tmp_bson ("{'$a': 1}"), true);

      r = (bool) mongoc_bulk_operation_execute (bulk, &reply, &error);
      ASSERT (!r);
      ASSERT_ERROR_CONTAINS (error,
                             MONGOC_ERROR_COMMAND,
                             MONGOC_ERROR_COMMAND_INVALID_ARG,
                             "replacement document contains invalid key: keys "
                             "cannot begin with \"$\": \"$a\"");
   }

   ASSERT (bson_empty (&reply));
   bson_destroy (&reply);
   mongoc_bulk_operation_destroy (bulk);

   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
}


static void
test_replace_one_check_keys (void)
{
   _test_replace_one_check_keys (false);
}


static void
test_replace_one_with_opts_check_keys (void)
{
   _test_replace_one_check_keys (true);
}

/*
 * check that we include command overhead in msg size when deciding to split,
 * CDRIVER-1082
 */
static void
test_upsert_large (void *ctx)
{
   mongoc_bulk_operation_t *bulk;
   mongoc_collection_t *collection;
   mongoc_client_t *client;
   bool has_write_cmds;
   bson_t *selector = tmp_bson ("{'_id': 'aaaaaaaaaa'}");
   size_t sz = 8396692; /* a little over 8 MB */
   char *large_str = bson_malloc (sz);
   bson_t update = BSON_INITIALIZER;
   bson_t child;
   bson_error_t error;
   int i;
   bson_t reply;

   memset (large_str, 'a', sz);
   large_str[sz - 1] = '\0';
   client = test_framework_client_new ();
   has_write_cmds = server_has_write_commands ();
   collection = get_test_collection (client, "test_upsert_large");
   bulk = mongoc_collection_create_bulk_operation (collection, true, NULL);

   bson_append_document_begin (&update, "$set", 4, &child);
   bson_append_utf8 (&child, "big", 3, large_str, (int) sz - 1);
   bson_append_document_end (&update, &child);

   /* two 8MB+ docs could fit in 16MB + 16K, if not for command overhead,
    * check the driver splits into two msgs */
   for (i = 0; i < 2; i++) {
      mongoc_bulk_operation_update (bulk, selector, &update, true);
   }

   ASSERT_OR_PRINT ((bool) mongoc_bulk_operation_execute (bulk, &reply, &error),
                    error);

   ASSERT_MATCH (&reply,
                 "{'nInserted': 0,"
                 " 'nMatched':  1,"
                 " 'nRemoved':  0,"
                 " 'nUpserted': 1,"
                 " 'upserted': [{'index': 0, '_id': 'aaaaaaaaaa'}],"
                 " 'writeErrors': []}");

   check_n_modified (has_write_cmds, &reply, 0);
   ASSERT_COUNT (1, collection);

   bson_destroy (&reply);
   mongoc_bulk_operation_destroy (bulk);
   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
   bson_destroy (&update);
   bson_free (large_str);
}


static void
test_upsert_huge (void *ctx)
{
   mongoc_bulk_operation_t *bulk;
   mongoc_collection_t *collection;
   mongoc_client_t *client;
   bool has_write_cmds;
   bson_t *sel = tmp_bson ("{'_id': 1}");
   bson_t doc = BSON_INITIALIZER;
   bson_t child = BSON_INITIALIZER;
   bson_t query = BSON_INITIALIZER;
   const bson_t *retdoc;
   bson_error_t error;
   bson_t reply;
   mongoc_cursor_t *cursor;

   client = test_framework_client_new ();
   BSON_ASSERT (client);
   mongoc_client_set_error_api (client, 2);
   has_write_cmds = server_has_write_commands ();

   collection = get_test_collection (client, "test_upsert_huge");
   BSON_ASSERT (collection);

   bulk = mongoc_collection_create_bulk_operation (collection, true, NULL);
   BSON_ASSERT (bulk);

   bson_append_document_begin (&doc, "$set", -1, &child);
   BSON_ASSERT (bson_append_utf8 (&child,
                                  "x",
                                  -1,
                                  huge_string (client),
                                  (int) huge_string_length (client)));
   bson_append_document_end (&doc, &child);

   mongoc_bulk_operation_update (bulk, sel, &doc, true);
   ASSERT_OR_PRINT ((bool) mongoc_bulk_operation_execute (bulk, &reply, &error),
                    error);

   ASSERT_MATCH (&reply,
                 "{'nInserted': 0,"
                 " 'nMatched':  0,"
                 " 'nRemoved':  0,"
                 " 'nUpserted': 1,"
                 " 'upserted':  [{'index': 0, '_id': 1}],"
                 " 'writeErrors': []}");

   check_n_modified (has_write_cmds, &reply, 0);
   ASSERT_COUNT (1, collection);

   cursor = mongoc_collection_find_with_opts (collection, &query, NULL, NULL);
   ASSERT_CURSOR_NEXT (cursor, &retdoc);
   ASSERT_CURSOR_DONE (cursor);

   bson_destroy (&query);
   bson_destroy (&reply);
   bson_destroy (&doc);
   mongoc_cursor_destroy (cursor);
   mongoc_bulk_operation_destroy (bulk);
   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
}


static void
test_replace_one_ordered (void)
{
   test_replace_one (true);
}


static void
test_replace_one_unordered (void)
{
   test_replace_one (false);
}


static void
test_update (bool ordered)
{
   mongoc_client_t *client;
   bool has_write_cmds;
   mongoc_collection_t *collection;
   bson_t *docs_inserted[] = {
      tmp_bson ("{'a': 1}"),
      tmp_bson ("{'a': 2}"),
      tmp_bson ("{'a': 3, 'foo': 'bar'}"),
   };
   unsigned int i;
   mongoc_bulk_operation_t *bulk;
   bson_error_t error;
   bson_t reply;
   bson_t *sel;
   bson_t *bad_update_doc = tmp_bson ("{'foo': 'bar'}");
   bson_t *update_doc;

   client = test_framework_client_new ();
   BSON_ASSERT (client);
   has_write_cmds = server_has_write_commands ();

   collection = get_test_collection (client, "test_update");
   BSON_ASSERT (collection);

   for (i = 0; i < sizeof docs_inserted / sizeof (bson_t *); i++) {
      BSON_ASSERT (mongoc_collection_insert (
         collection, MONGOC_INSERT_NONE, docs_inserted[i], NULL, NULL));
   }

   bulk = mongoc_collection_create_bulk_operation (collection, ordered, NULL);
   BSON_ASSERT (bulk);

   /* an update doc without $-operators is rejected */
   sel = tmp_bson ("{'a': {'$gte': 2}}");
   capture_logs (true);
   mongoc_bulk_operation_update (bulk, sel, bad_update_doc, false);

   BSON_ASSERT (!mongoc_bulk_operation_execute (bulk, &reply, &error));
   ASSERT_ERROR_CONTAINS (
      error,
      MONGOC_ERROR_COMMAND,
      MONGOC_ERROR_COMMAND_INVALID_ARG,
      "Invalid key 'foo': update only works with $ operators");

   BSON_ASSERT (bson_empty (&reply));
   mongoc_bulk_operation_destroy (bulk);
   bson_destroy (&reply);

   bulk = mongoc_collection_create_bulk_operation (collection, ordered, NULL);
   update_doc = tmp_bson ("{'$set': {'foo': 'bar'}}");
   mongoc_bulk_operation_update (bulk, sel, update_doc, false);
   ASSERT_OR_PRINT (mongoc_bulk_operation_execute (bulk, &reply, &error),
                    error);

   ASSERT_MATCH (&reply,
                 "{'nInserted': 0,"
                 " 'nMatched':  2,"
                 " 'nRemoved':  0,"
                 " 'nUpserted': 0,"
                 " 'upserted':  {'$exists': false},"
                 " 'writeErrors': []}");

   /* one doc already had "foo": "bar" */
   check_n_modified (has_write_cmds, &reply, 1);
   ASSERT_COUNT (3, collection);

   ASSERT_OR_PRINT (mongoc_collection_drop (collection, &error), error);

   mongoc_bulk_operation_destroy (bulk);
   bson_destroy (&reply);
   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
}


static void
test_update_ordered (void)
{
   test_update (true);
}


static void
test_update_unordered (void)
{
   test_update (false);
}


/* update document has key that doesn't start with "$" */
static void
_test_update_check_keys (bool many, bool with_opts)
{
   mongoc_bulk_operation_t *bulk;
   mongoc_collection_t *collection;
   mongoc_client_t *client;
   bson_t *q = tmp_bson ("{}");
   bson_t *u = tmp_bson ("{'a': 1}");
   bson_t reply;
   bson_error_t error;
   bool r;

   client = test_framework_client_new ();
   BSON_ASSERT (client);
   collection = get_test_collection (client, "test_update_check_keys");
   BSON_ASSERT (collection);

   bulk = mongoc_collection_create_bulk_operation (collection, true, NULL);
   BSON_ASSERT (bulk);

   capture_logs (true);

   if (with_opts) {
      /* document is rejected immediately */
      if (many) {
         r = mongoc_bulk_operation_update_many_with_opts (
            bulk, q, u, NULL, &error);
      } else {
         r = mongoc_bulk_operation_update_one_with_opts (
            bulk, q, u, NULL, &error);
      }
      BSON_ASSERT (!r);
      ASSERT_ERROR_CONTAINS (
         error,
         MONGOC_ERROR_COMMAND,
         MONGOC_ERROR_COMMAND_INVALID_ARG,
         "Invalid key 'a': update only works with $ operators");

      r = (bool) mongoc_bulk_operation_execute (bulk, &reply, &error);
      BSON_ASSERT (!r);
      ASSERT_ERROR_CONTAINS (error,
                             MONGOC_ERROR_COMMAND,
                             MONGOC_ERROR_COMMAND_INVALID_ARG,
                             "empty bulk");
   } else {
      /* document rejected when bulk op is executed */
      if (many) {
         mongoc_bulk_operation_update (bulk, q, u, false);
      } else {
         mongoc_bulk_operation_update_one (bulk, q, u, false);
      }
      r = (bool) mongoc_bulk_operation_execute (bulk, &reply, &error);
      BSON_ASSERT (!r);
      ASSERT_ERROR_CONTAINS (
         error,
         MONGOC_ERROR_COMMAND,
         MONGOC_ERROR_COMMAND_INVALID_ARG,
         "Invalid key 'a': update only works with $ operators");
   }

   BSON_ASSERT (bson_empty (&reply));

   bson_destroy (&reply);
   mongoc_bulk_operation_destroy (bulk);
   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
}


static void
test_update_one_check_keys (void)
{
   _test_update_check_keys (false, false);
}


static void
test_update_check_keys (void)
{
   _test_update_check_keys (true, false);
}


static void
test_update_one_with_opts_check_keys (void)
{
   _test_update_check_keys (false, true);
}


static void
test_update_many_with_opts_check_keys (void)
{
   _test_update_check_keys (true, true);
}


typedef void (*update_fn) (mongoc_bulk_operation_t *bulk,
                           const bson_t *selector,
                           const bson_t *document,
                           bool upsert);

typedef bool (*update_with_opts_fn) (mongoc_bulk_operation_t *bulk,
                                     const bson_t *selector,
                                     const bson_t *document,
                                     const bson_t *opts,
                                     bson_error_t *error);

typedef struct {
   const char *bad_update_json;
   const char *good_update_json;
   update_fn update;
   update_with_opts_fn update_with_opts;
   bool invalid_first;
   const char *error_message;
} update_validate_test_t;


static void
_test_update_validate (update_validate_test_t *test)
{
   mongoc_bulk_operation_t *bulk;
   mongoc_collection_t *collection;
   mongoc_client_t *client;
   bson_t *q = tmp_bson ("{}");
   bson_t *bad_update = tmp_bson (test->bad_update_json);
   bson_t *good_update = tmp_bson (test->good_update_json);
   bson_t reply;
   bson_error_t error;
   bool r;

   client = test_framework_client_new ();
   BSON_ASSERT (client);
   collection = get_test_collection (client, "test_update_invalid_first");
   BSON_ASSERT (collection);

   bulk = mongoc_collection_create_bulk_operation (collection, true, NULL);
   BSON_ASSERT (bulk);

   capture_logs (true);

   if (test->update_with_opts) {
      if (test->invalid_first) {
         /* document is rejected immediately */
         r = test->update_with_opts (bulk, q, bad_update, NULL, &error);
         BSON_ASSERT (!r);
         ASSERT_ERROR_CONTAINS (error,
                                MONGOC_ERROR_COMMAND,
                                MONGOC_ERROR_COMMAND_INVALID_ARG,
                                test->error_message);

         /* now a valid document */
         r = test->update_with_opts (bulk, q, good_update, NULL, &error);
         ASSERT_OR_PRINT (r, error);
         ASSERT_CMPSIZE_T ((size_t) 1, ==, bulk->commands.len);
         r = (bool) mongoc_bulk_operation_execute (bulk, &reply, &error);
         ASSERT_OR_PRINT (r, error);
         BSON_ASSERT (!bson_empty (&reply));
      } else {
         /* first a valid document */
         r = test->update_with_opts (bulk, q, good_update, NULL, &error);
         ASSERT_OR_PRINT (r, error);

         /* invalid document is rejected without invalidating batch */
         r = test->update_with_opts (bulk, q, bad_update, NULL, &error);
         BSON_ASSERT (!r);
         ASSERT_ERROR_CONTAINS (error,
                                MONGOC_ERROR_COMMAND,
                                MONGOC_ERROR_COMMAND_INVALID_ARG,
                                test->error_message);

         ASSERT_CMPSIZE_T ((size_t) 1, ==, bulk->commands.len);
         r = (bool) mongoc_bulk_operation_execute (bulk, &reply, &error);
         ASSERT_OR_PRINT (r, error);
         BSON_ASSERT (!bson_empty (&reply));
      }
   } else {
      if (test->invalid_first) {
         /* invalid, then valid */
         test->update (bulk, q, bad_update, false);
         test->update (bulk, q, good_update, false);

         /* not added */
         ASSERT_CMPSIZE_T ((size_t) 0, ==, bulk->commands.len);

         /* invalid document invalidated the whole bulk */
         r = (bool) mongoc_bulk_operation_execute (bulk, &reply, &error);
         BSON_ASSERT (!r);
         BSON_ASSERT (bson_empty (&reply));
         ASSERT_ERROR_CONTAINS (error,
                                MONGOC_ERROR_COMMAND,
                                MONGOC_ERROR_COMMAND_INVALID_ARG,
                                test->error_message);
      } else {
         /* valid, then invalid */
         test->update (bulk, q, good_update, false);
         test->update (bulk, q, bad_update, false);

         ASSERT_CMPSIZE_T ((size_t) 1, ==, bulk->commands.len);

         /* invalid document invalidated the whole bulk */
         r = (bool) mongoc_bulk_operation_execute (bulk, &reply, &error);
         BSON_ASSERT (!r);
         BSON_ASSERT (bson_empty (&reply));
         ASSERT_ERROR_CONTAINS (error,
                                MONGOC_ERROR_COMMAND,
                                MONGOC_ERROR_COMMAND_INVALID_ARG,
                                test->error_message);
      }
   }

   bson_destroy (&reply);
   mongoc_bulk_operation_destroy (bulk);
   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
}


static void
_test_update_one_invalid (bool first)
{
   update_validate_test_t test = {0};
   test.bad_update_json = "{'a': 1}";
   test.good_update_json = "{'$set': {'x': 1}}";
   test.update = mongoc_bulk_operation_update_one;
   test.update_with_opts = NULL;
   test.invalid_first = first;
   test.error_message = "Invalid key 'a': update only works with $ operators";

   _test_update_validate (&test);
}


static void
_test_update_invalid (bool first)
{
   update_validate_test_t test = {0};
   test.bad_update_json = "{'a': 1}";
   test.good_update_json = "{'$set': {'x': 1}}";
   test.update = mongoc_bulk_operation_update;
   test.update_with_opts = NULL;
   test.invalid_first = first;
   test.error_message = "Invalid key 'a': update only works with $ operators";

   _test_update_validate (&test);
}


static void
_test_update_one_with_opts_invalid (bool first)
{
   update_validate_test_t test = {0};
   test.bad_update_json = "{'a': 1}";
   test.good_update_json = "{'$set': {'x': 1}}";
   test.update = NULL;
   test.update_with_opts = mongoc_bulk_operation_update_one_with_opts;
   test.invalid_first = first;
   test.error_message = "Invalid key 'a': update only works with $ operators";

   _test_update_validate (&test);
}


static void
_test_update_many_with_opts_invalid (bool first)
{
   update_validate_test_t test = {0};
   test.bad_update_json = "{'a': 1}";
   test.good_update_json = "{'$set': {'x': 1}}";
   test.update = NULL;
   test.update_with_opts = mongoc_bulk_operation_update_many_with_opts;
   test.invalid_first = first;
   test.error_message = "Invalid key 'a': update only works with $ operators";

   _test_update_validate (&test);
}


static void
_test_replace_one_invalid (bool first)
{
   update_validate_test_t test = {0};
   test.bad_update_json = "{'$set': {'x': 1}}";
   test.good_update_json = "{'a': 1}";
   test.update = mongoc_bulk_operation_replace_one;
   test.update_with_opts = NULL;
   test.invalid_first = first;
   test.error_message = "replacement document contains invalid key: keys "
                        "cannot begin with \"$\": \"$set\"";

   _test_update_validate (&test);
}


static void
_test_replace_one_with_opts_invalid (bool first)
{
   update_validate_test_t test = {0};
   test.bad_update_json = "{'$set': {'x': 1}}";
   test.good_update_json = "{'a': 1}";
   test.update = NULL;
   test.update_with_opts = mongoc_bulk_operation_replace_one_with_opts;
   test.invalid_first = first;
   test.error_message = "replacement document contains invalid key: keys "
                        "cannot begin with \"$\": \"$set\"";

   _test_update_validate (&test);
}


static void
test_update_one_invalid_first (void)
{
   _test_update_one_invalid (true /* invalid first */);
}


static void
test_update_invalid_first (void)
{
   _test_update_invalid (true /* invalid first */);
}


static void
test_update_one_with_opts_invalid_first (void)
{
   _test_update_one_with_opts_invalid (true /* invalid first */);
}


static void
test_update_many_with_opts_invalid_first (void)
{
   _test_update_many_with_opts_invalid (true /* invalid first */);
}


static void
test_replace_one_invalid_first (void)
{
   _test_replace_one_invalid (true /* invalid first */);
}


static void
test_replace_one_with_opts_invalid_first (void)
{
   _test_replace_one_with_opts_invalid (true /* invalid first */);
}


static void
test_update_one_invalid_second (void)
{
   _test_update_one_invalid (false /* invalid first */);
}


static void
test_update_invalid_second (void)
{
   _test_update_invalid (false /* invalid first */);
}


static void
test_update_one_with_opts_invalid_second (void)
{
   _test_update_one_with_opts_invalid (false /* invalid first */);
}


static void
test_update_many_with_opts_invalid_second (void)
{
   _test_update_many_with_opts_invalid (false /* invalid first */);
}


static void
test_replace_one_invalid_second (void)
{
   _test_replace_one_invalid (false /* invalid first */);
}


static void
test_replace_one_with_opts_invalid_second (void)
{
   _test_replace_one_with_opts_invalid (false /* invalid first */);
}


static void
_test_insert_invalid (bool with_opts, bool invalid_first)
{
   mongoc_bulk_operation_t *bulk;
   mongoc_collection_t *collection;
   mongoc_client_t *client;
   bson_t *bad_insert = tmp_bson ("{'a.b': 1}");
   bson_t *good_insert = tmp_bson ("{'x': 1}");
   bson_t reply;
   bson_error_t error;
   bool r;
   const char *err = "document to insert contains invalid key: keys cannot "
                     "contain \".\": \"a.b\"";

   client = test_framework_client_new ();
   collection = get_test_collection (client, "test_insert_validate");
   bulk = mongoc_collection_create_bulk_operation (collection, true, NULL);
   BSON_ASSERT (mongoc_collection_remove (
      collection, MONGOC_REMOVE_NONE, tmp_bson (NULL), NULL, NULL));

   capture_logs (true);

   if (with_opts) {
      if (invalid_first) {
         /* document is rejected immediately */
         r = mongoc_bulk_operation_insert_with_opts (
            bulk, bad_insert, NULL, &error);

         BSON_ASSERT (!r);
         ASSERT_ERROR_CONTAINS (
            error, MONGOC_ERROR_COMMAND, MONGOC_ERROR_COMMAND_INVALID_ARG, err);

         /* now a valid document */
         r = mongoc_bulk_operation_insert_with_opts (
            bulk, good_insert, NULL, &error);
         ASSERT_OR_PRINT (r, error);
         ASSERT_CMPSIZE_T ((size_t) 1, ==, bulk->commands.len);
         r = (bool) mongoc_bulk_operation_execute (bulk, &reply, &error);
         ASSERT_OR_PRINT (r, error);
         BSON_ASSERT (!bson_empty (&reply));
      } else {
         /* first a valid document */
         r = mongoc_bulk_operation_insert_with_opts (
            bulk, good_insert, NULL, &error);
         ASSERT_OR_PRINT (r, error);

         /* invalid document is rejected without invalidating batch */
         r = mongoc_bulk_operation_insert_with_opts (
            bulk, bad_insert, NULL, &error);
         BSON_ASSERT (!r);
         ASSERT_ERROR_CONTAINS (
            error, MONGOC_ERROR_COMMAND, MONGOC_ERROR_COMMAND_INVALID_ARG, err);

         ASSERT_CMPSIZE_T ((size_t) 1, ==, bulk->commands.len);
         r = (bool) mongoc_bulk_operation_execute (bulk, &reply, &error);
         ASSERT_OR_PRINT (r, error);
         BSON_ASSERT (!bson_empty (&reply));
      }
   } else { /* not "with_opts" */
      if (invalid_first) {
         /* invalid, then valid */
         mongoc_bulk_operation_insert (bulk, bad_insert);
         mongoc_bulk_operation_insert (bulk, good_insert);

         /* not added */
         ASSERT_CMPSIZE_T ((size_t) 0, ==, bulk->commands.len);

         /* invalid document invalidated the whole bulk */
         r = (bool) mongoc_bulk_operation_execute (bulk, &reply, &error);
         BSON_ASSERT (!r);
         BSON_ASSERT (bson_empty (&reply));
         ASSERT_ERROR_CONTAINS (
            error, MONGOC_ERROR_COMMAND, MONGOC_ERROR_COMMAND_INVALID_ARG, err);
      } else {
         /* valid, then invalid */
         mongoc_bulk_operation_insert (bulk, good_insert);
         mongoc_bulk_operation_insert (bulk, bad_insert);

         ASSERT_CMPSIZE_T ((size_t) 1, ==, bulk->commands.len);

         /* invalid document invalidated the whole bulk */
         r = (bool) mongoc_bulk_operation_execute (bulk, &reply, &error);
         BSON_ASSERT (!r);
         BSON_ASSERT (bson_empty (&reply));
         ASSERT_ERROR_CONTAINS (
            error, MONGOC_ERROR_COMMAND, MONGOC_ERROR_COMMAND_INVALID_ARG, err);
      }
   }

   bson_destroy (&reply);
   mongoc_bulk_operation_destroy (bulk);
   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
}


static void
test_insert_invalid_first (void)
{
   _test_insert_invalid (true, false);
}


static void
test_insert_invalid_second (void)
{
   _test_insert_invalid (false, false);
}


static void
test_insert_with_opts_invalid_first (void)
{
   _test_insert_invalid (true, true);
}


static void
test_insert_with_opts_invalid_second (void)
{
   _test_insert_invalid (false, true);
}


/* CDRIVER-2018, special dispensation for PHP driver and system.indexes */
static void
test_insert_into_system_indexes (void)
{
   mongoc_client_t *client;
   const char *db_name;
   mongoc_collection_t *collection;
   mongoc_collection_t *system_indexes;
   mongoc_bulk_operation_t *bulk;
   bool r;
   bson_error_t error;
   uint32_t i;
   mongoc_cursor_t *cursor;
   const bson_t *index_info;
   bson_iter_t iter;
   bool found = false;

   client = test_framework_client_new ();
   collection = get_test_collection (client, "test_insert_system_indexes");
   db_name = collection->db;
   system_indexes =
      mongoc_client_get_collection (client, db_name, "system.indexes");

   mongoc_collection_drop (collection, NULL);

   bulk = mongoc_collection_create_bulk_operation (system_indexes, false, NULL);
   r = mongoc_bulk_operation_insert_with_opts (
      bulk,
      tmp_bson ("{'key': {'a.b': 1}, 'ns': '%s', 'name': 'foo'}",
                collection->ns),
      tmp_bson ("{'legacyIndex': true}"),
      &error);

   ASSERT_OR_PRINT (r, error);

   /* even modern MongoDB lets us insert into system.indexes to create index */
   i = mongoc_bulk_operation_execute (bulk, NULL, &error);
   ASSERT_OR_PRINT (i, error);
   cursor = mongoc_collection_find_indexes (collection, &error);
   ASSERT_OR_PRINT (cursor, error);
   while (mongoc_cursor_next (cursor, &index_info)) {
      if (bson_iter_init_find (&iter, index_info, "name") &&
          !strcmp (bson_iter_utf8 (&iter, NULL), "foo")) {
         found = true;
         break;
      }
   }

   r = mongoc_cursor_error (cursor, &error);
   ASSERT_OR_PRINT (!r, error);
   BSON_ASSERT (found);

   mongoc_cursor_destroy (cursor);

   mongoc_bulk_operation_destroy (bulk);
   mongoc_collection_destroy (system_indexes);
   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
}


typedef void (*remove_fn) (mongoc_bulk_operation_t *bulk,
                           const bson_t *selector);

typedef bool (*remove_with_opts_fn) (mongoc_bulk_operation_t *bulk,
                                     const bson_t *selector,
                                     const bson_t *opts,
                                     bson_error_t *error);

typedef struct {
   remove_fn remove;
   remove_with_opts_fn remove_with_opts;
} remove_validate_test_t;


static void
_test_remove_validate (remove_validate_test_t *test)
{
   mongoc_bulk_operation_t *bulk;
   mongoc_collection_t *collection;
   mongoc_client_t *client;
   bson_t reply;
   bson_error_t error;
   bool r;

   client = test_framework_client_new ();
   BSON_ASSERT (client);
   collection = get_test_collection (client, "test_update_invalid_first");
   BSON_ASSERT (collection);

   bulk = mongoc_collection_create_bulk_operation (collection, true, NULL);
   BSON_ASSERT (bulk);

   capture_logs (true);

   /* invalid */
   mongoc_bulk_operation_insert (bulk, tmp_bson ("{'$a': 1}"));

   if (test->remove_with_opts) {
      r = test->remove_with_opts (bulk, tmp_bson (NULL), NULL, &error);
      BSON_ASSERT (!r);
      ASSERT_ERROR_CONTAINS (error,
                             MONGOC_ERROR_COMMAND,
                             MONGOC_ERROR_COMMAND_INVALID_ARG,
                             "Bulk operation is invalid from prior error: "
                             "document to insert contains invalid key: keys "
                             "cannot begin with \"$\": \"$a\"");
   } else {
      test->remove (bulk, tmp_bson (NULL));
   }

   /* remove operation was not recorded */
   ASSERT_CMPSIZE_T ((size_t) 0, ==, bulk->commands.len);

   /* invalid document invalidated the whole bulk */
   r = (bool) mongoc_bulk_operation_execute (bulk, &reply, &error);
   BSON_ASSERT (!r);
   BSON_ASSERT (bson_empty (&reply));
   ASSERT_ERROR_CONTAINS (error,
                          MONGOC_ERROR_COMMAND,
                          MONGOC_ERROR_COMMAND_INVALID_ARG,
                          "document to insert contains invalid key: keys "
                          "cannot begin with \"$\": \"$a\"");

   bson_destroy (&reply);
   mongoc_bulk_operation_destroy (bulk);
   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
}


static void
test_remove_one_after_invalid (void)
{
   remove_validate_test_t test = {0};
   test.remove = mongoc_bulk_operation_remove_one;

   _test_remove_validate (&test);
}
static void
test_remove_after_invalid (void)
{
   remove_validate_test_t test = {0};
   test.remove = mongoc_bulk_operation_remove;

   _test_remove_validate (&test);
}
static void
test_remove_one_with_opts_after_invalid (void)
{
   remove_validate_test_t test = {0};
   test.remove_with_opts = mongoc_bulk_operation_remove_one_with_opts;

   _test_remove_validate (&test);
}
static void
test_remove_many_with_opts_after_invalid (void)
{
   remove_validate_test_t test = {0};
   test.remove_with_opts = mongoc_bulk_operation_remove_many_with_opts;

   _test_remove_validate (&test);
}

static void
test_index_offset (void)
{
   mongoc_bulk_operation_t *bulk;
   mongoc_collection_t *collection;
   mongoc_client_t *client;
   bool has_write_cmds;
   bson_error_t error;
   bson_t reply;
   bson_t *sel;
   bson_t *doc;
   bool r;

   client = test_framework_client_new ();
   BSON_ASSERT (client);
   has_write_cmds = server_has_write_commands ();

   collection = get_test_collection (client, "test_index_offset");
   BSON_ASSERT (collection);

   doc = tmp_bson ("{}");
   BSON_APPEND_INT32 (doc, "_id", 1234);
   r = mongoc_collection_insert (
      collection, MONGOC_INSERT_NONE, doc, NULL, &error);
   BSON_ASSERT (r);

   bulk = mongoc_collection_create_bulk_operation (collection, true, NULL);
   BSON_ASSERT (bulk);

   sel = tmp_bson ("{'_id': 1234}");
   doc = tmp_bson ("{'$set': {'hello': 'there'}}");

   mongoc_bulk_operation_remove_one (bulk, sel);
   mongoc_bulk_operation_update (bulk, sel, doc, true);

   ASSERT_OR_PRINT (mongoc_bulk_operation_execute (bulk, &reply, &error),
                    error);

   ASSERT_MATCH (&reply,
                 "{'nInserted': 0,"
                 " 'nMatched':  0,"
                 " 'nRemoved':  1,"
                 " 'nUpserted': 1,"
                 " 'upserted': [{'index': 1, '_id': 1234}],"
                 " 'writeErrors': []}");

   check_n_modified (has_write_cmds, &reply, 0);
   ASSERT_COUNT (1, collection);

   bson_destroy (&reply);

   ASSERT_OR_PRINT (mongoc_collection_drop (collection, &error), error);

   mongoc_bulk_operation_destroy (bulk);
   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
}


static void
test_single_ordered_bulk (void)
{
   mongoc_client_t *client;
   bool has_write_cmds;
   mongoc_collection_t *collection;
   mongoc_bulk_operation_t *bulk;
   bson_t reply;
   bson_error_t error;

   client = test_framework_client_new ();
   BSON_ASSERT (client);
   has_write_cmds = server_has_write_commands ();

   collection = get_test_collection (client, "test_single_ordered_bulk");
   BSON_ASSERT (collection);

   bulk = mongoc_collection_create_bulk_operation (collection, true, NULL);
   BSON_ASSERT (bulk);

   mongoc_bulk_operation_insert (bulk, tmp_bson ("{'a': 1}"));
   mongoc_bulk_operation_update (
      bulk, tmp_bson ("{'a': 1}"), tmp_bson ("{'$set': {'b': 1}}"), false);
   mongoc_bulk_operation_update (
      bulk, tmp_bson ("{'a': 2}"), tmp_bson ("{'$set': {'b': 2}}"), true);
   mongoc_bulk_operation_insert (bulk, tmp_bson ("{'a': 3}"));
   mongoc_bulk_operation_remove (bulk, tmp_bson ("{'a': 3}"));
   ASSERT_OR_PRINT ((bool) mongoc_bulk_operation_execute (bulk, &reply, &error),
                    error);

   ASSERT_MATCH (&reply,
                 "{'nInserted': 2,"
                 " 'nMatched':  1,"
                 " 'nRemoved':  1,"
                 " 'nUpserted': 1,"
                 " 'upserted': [{'index': 2, '_id': {'$exists': true}}]"
                 "}");

   check_n_modified (has_write_cmds, &reply, 1);
   ASSERT_COUNT (2, collection);

   bson_destroy (&reply);
   mongoc_bulk_operation_destroy (bulk);
   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
}


static void
test_insert_continue_on_error (void)
{
   mongoc_client_t *client;
   bool has_write_cmds;
   mongoc_collection_t *collection;
   mongoc_bulk_operation_t *bulk;
   bson_t *doc0 = tmp_bson ("{'a': 1}");
   bson_t *doc1 = tmp_bson ("{'a': 2}");
   bson_t reply;
   bson_error_t error;
   bool r;

   client = test_framework_client_new ();
   BSON_ASSERT (client);
   has_write_cmds = server_has_write_commands ();

   collection = get_test_collection (client, "test_insert_continue_on_error");
   BSON_ASSERT (collection);

   create_unique_index (collection);

   bulk = mongoc_collection_create_bulk_operation (collection, false, NULL);
   mongoc_bulk_operation_insert (bulk, doc0);
   mongoc_bulk_operation_insert (bulk, doc0);
   mongoc_bulk_operation_insert (bulk, doc1);
   mongoc_bulk_operation_insert (bulk, doc1);
   r = (bool) mongoc_bulk_operation_execute (bulk, &reply, &error);
   BSON_ASSERT (!r);

   ASSERT_MATCH (&reply,
                 "{'nInserted': 2,"
                 " 'nMatched':  0,"
                 " 'nRemoved':  0,"
                 " 'nUpserted': 0,"
                 " 'writeErrors': [{'index': 1}, {'index': 3}]}");

   check_n_modified (has_write_cmds, &reply, 0);
   assert_error_count (2, &reply);
   ASSERT_COUNT (2, collection);

   bson_destroy (&reply);
   mongoc_bulk_operation_destroy (bulk);
   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
}


static void
test_update_continue_on_error (void)
{
   mongoc_client_t *client;
   bool has_write_cmds;
   mongoc_collection_t *collection;
   mongoc_bulk_operation_t *bulk;
   bson_t *doc0 = tmp_bson ("{'a': 1}");
   bson_t *doc1 = tmp_bson ("{'a': 2}");
   bson_t reply;
   bson_error_t error;
   bool r;

   client = test_framework_client_new ();
   BSON_ASSERT (client);
   has_write_cmds = server_has_write_commands ();

   collection = get_test_collection (client, "test_update_continue_on_error");
   BSON_ASSERT (collection);

   create_unique_index (collection);
   mongoc_collection_insert (collection, MONGOC_INSERT_NONE, doc0, NULL, NULL);
   mongoc_collection_insert (collection, MONGOC_INSERT_NONE, doc1, NULL, NULL);

   bulk = mongoc_collection_create_bulk_operation (collection, false, NULL);
   /* succeeds */
   mongoc_bulk_operation_update (
      bulk, doc0, tmp_bson ("{'$inc': {'b': 1}}"), false);
   /* fails */
   mongoc_bulk_operation_update (
      bulk, doc0, tmp_bson ("{'$set': {'a': 2}}"), false);
   /* succeeds */
   mongoc_bulk_operation_update (
      bulk, doc1, tmp_bson ("{'$set': {'b': 2}}"), false);

   r = (bool) mongoc_bulk_operation_execute (bulk, &reply, &error);
   BSON_ASSERT (!r);

   ASSERT_MATCH (&reply,
                 "{'nInserted': 0,"
                 " 'nMatched':  2,"
                 " 'nRemoved':  0,"
                 " 'nUpserted': 0,"
                 " 'writeErrors': [{'index': 1}]}");

   check_n_modified (has_write_cmds, &reply, 2);
   assert_error_count (1, &reply);
   ASSERT_COUNT (2, collection);
   ASSERT_CMPINT (1,
                  ==,
                  (int) mongoc_collection_count (collection,
                                                 MONGOC_QUERY_NONE,
                                                 tmp_bson ("{'b': 2}"),
                                                 0,
                                                 0,
                                                 NULL,
                                                 NULL));

   bson_destroy (&reply);
   mongoc_bulk_operation_destroy (bulk);
   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
}


static void
test_remove_continue_on_error (void)
{
   mongoc_client_t *client;
   bool has_write_cmds;
   mongoc_collection_t *collection;
   mongoc_bulk_operation_t *bulk;
   bson_t *doc0 = tmp_bson ("{'a': 1}");
   bson_t *doc1 = tmp_bson ("{'a': 2}");
   bson_t *doc2 = tmp_bson ("{'a': 3}");
   bson_t reply;
   bson_error_t error;
   bool r;

   client = test_framework_client_new ();
   BSON_ASSERT (client);
   has_write_cmds = server_has_write_commands ();

   collection = get_test_collection (client, "test_remove_continue_on_error");
   BSON_ASSERT (collection);

   mongoc_collection_insert (collection, MONGOC_INSERT_NONE, doc0, NULL, NULL);
   mongoc_collection_insert (collection, MONGOC_INSERT_NONE, doc1, NULL, NULL);
   mongoc_collection_insert (collection, MONGOC_INSERT_NONE, doc2, NULL, NULL);

   bulk = mongoc_collection_create_bulk_operation (collection, false, NULL);
   /* succeeds */
   mongoc_bulk_operation_remove_one (bulk, doc0);
   /* fails */
   mongoc_bulk_operation_remove_one (bulk, tmp_bson ("{'a': {'$bad': 1}}"));
   /* succeeds */
   mongoc_bulk_operation_remove_one (bulk, doc1);

   r = (bool) mongoc_bulk_operation_execute (bulk, &reply, &error);
   BSON_ASSERT (!r);

   ASSERT_MATCH (&reply,
                 "{'nInserted': 0,"
                 " 'nMatched':  0,"
                 " 'nRemoved':  2,"
                 " 'nUpserted': 0,"
                 " 'writeErrors': [{'index': 1}]}");

   check_n_modified (has_write_cmds, &reply, 0);
   assert_error_count (1, &reply);
   ASSERT_COUNT (1, collection);

   bson_destroy (&reply);
   mongoc_bulk_operation_destroy (bulk);
   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
}


static void
test_single_error_ordered_bulk (void)
{
   mongoc_client_t *client;
   bool has_write_cmds;
   mongoc_collection_t *collection;
   mongoc_bulk_operation_t *bulk;
   bson_t reply;
   bson_error_t error;
   bool r;

   client = test_framework_client_new ();
   BSON_ASSERT (client);
   has_write_cmds = server_has_write_commands ();

   collection = get_test_collection (client, "test_single_error_ordered_bulk");
   BSON_ASSERT (collection);

   create_unique_index (collection);

   bulk = mongoc_collection_create_bulk_operation (collection, true, NULL);
   BSON_ASSERT (bulk);
   mongoc_bulk_operation_insert (bulk, tmp_bson ("{'b': 1, 'a': 1}"));
   mongoc_bulk_operation_update (
      bulk, tmp_bson ("{'b': 2}"), tmp_bson ("{'$set': {'a': 1}}"), true);
   mongoc_bulk_operation_insert (bulk, tmp_bson ("{'b': 3, 'a': 2}"));

   r = (bool) mongoc_bulk_operation_execute (bulk, &reply, &error);
   BSON_ASSERT (!r);
   ASSERT_CMPINT (error.domain, ==, MONGOC_ERROR_COMMAND);

   /* TODO: CDRIVER-651, BSON_ASSERT contents of the 'op' field */
   ASSERT_MATCH (&reply,
                 "{'nInserted': 1,"
                 " 'nMatched':  0,"
                 " 'nRemoved':  0,"
                 " 'nUpserted': 0,"
                 " 'writeErrors': ["
                 "    {'index': 1,"
                 "     'code':   {'$exists': true},"
                 "     'errmsg': {'$exists': true}}]"
                 /*
                  *                       " 'writeErrors.0.op':     ...,"
                  */
                 "}");
   assert_error_count (1, &reply);
   check_n_modified (has_write_cmds, &reply, 0);
   ASSERT_COUNT (1, collection);

   bson_destroy (&reply);
   mongoc_bulk_operation_destroy (bulk);
   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
}


static void
test_multiple_error_ordered_bulk (void)
{
   mongoc_client_t *client;
   bool has_write_cmds;
   mongoc_collection_t *collection;
   mongoc_bulk_operation_t *bulk;
   bson_t reply;
   bson_error_t error;
   bool r;

   client = test_framework_client_new ();
   BSON_ASSERT (client);
   has_write_cmds = server_has_write_commands ();

   collection =
      get_test_collection (client, "test_multiple_error_ordered_bulk");
   BSON_ASSERT (collection);

   create_unique_index (collection);

   bulk = mongoc_collection_create_bulk_operation (collection, true, NULL);
   BSON_ASSERT (bulk);

   /* 0 succeeds */
   mongoc_bulk_operation_insert (bulk, tmp_bson ("{'b': 1, 'a': 1}"));
   /* 1 succeeds */
   mongoc_bulk_operation_update (
      bulk, tmp_bson ("{'b': 3}"), tmp_bson ("{'$set': {'a': 2}}"), true);
   /* 2 fails, duplicate value for 'a' */
   mongoc_bulk_operation_update (
      bulk, tmp_bson ("{'b': 2}"), tmp_bson ("{'$set': {'a': 1}}"), true);
   /* 3 not attempted, bulk is already aborted */
   mongoc_bulk_operation_insert (bulk, tmp_bson ("{'b': 4, 'a': 3}"));

   r = (bool) mongoc_bulk_operation_execute (bulk, &reply, &error);
   BSON_ASSERT (!r);
   ASSERT_CMPINT (error.domain, ==, MONGOC_ERROR_COMMAND);
   BSON_ASSERT (error.code);

   /* TODO: CDRIVER-651, BSON_ASSERT contents of the 'op' field */
   ASSERT_MATCH (&reply,
                 "{'nInserted': 1,"
                 " 'nMatched':  0,"
                 " 'nRemoved':  0,"
                 " 'nUpserted': 1,"
                 " 'writeErrors': ["
                 "    {'index': 2, 'errmsg': {'$exists': true}}"
                 "]"
                 /*
                  *                       " 'writeErrors.0.op': {'q': {'b': 2},
                  * 'u': {'$set': {'a': 1}}, 'multi': false}"
                  */
                 "}");
   check_n_modified (has_write_cmds, &reply, 0);
   assert_error_count (1, &reply);
   ASSERT_COUNT (2, collection);

   bson_destroy (&reply);
   mongoc_bulk_operation_destroy (bulk);
   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
}


static void
test_single_unordered_bulk (void)
{
   mongoc_client_t *client;
   bool has_write_cmds;
   mongoc_collection_t *collection;
   mongoc_bulk_operation_t *bulk;
   bson_t reply;
   bson_error_t error;

   client = test_framework_client_new ();
   BSON_ASSERT (client);
   has_write_cmds = server_has_write_commands ();

   collection = get_test_collection (client, "test_single_unordered_bulk");
   BSON_ASSERT (collection);

   bulk = mongoc_collection_create_bulk_operation (collection, false, NULL);
   mongoc_bulk_operation_insert (bulk, tmp_bson ("{'a': 1}"));
   mongoc_bulk_operation_update (
      bulk, tmp_bson ("{'a': 1}"), tmp_bson ("{'$set': {'b': 1}}"), false);
   mongoc_bulk_operation_update (
      bulk, tmp_bson ("{'a': 2}"), tmp_bson ("{'$set': {'b': 2}}"), true);
   mongoc_bulk_operation_insert (bulk, tmp_bson ("{'a': 3}"));
   mongoc_bulk_operation_remove (bulk, tmp_bson ("{'a': 3}"));
   ASSERT_OR_PRINT ((bool) mongoc_bulk_operation_execute (bulk, &reply, &error),
                    error);

   ASSERT_MATCH (&reply,
                 "{'nInserted': 2,"
                 " 'nMatched': 1,"
                 " 'nRemoved': 1,"
                 " 'nUpserted': 1,"
                 " 'upserted': ["
                 "    {'index': 2, '_id': {'$exists': true}}],"
                 " 'writeErrors': []}");
   check_n_modified (has_write_cmds, &reply, 1);
   ASSERT_COUNT (2, collection);

   bson_destroy (&reply);
   mongoc_bulk_operation_destroy (bulk);
   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
}


static void
test_single_error_unordered_bulk (void)
{
   mongoc_client_t *client;
   bool has_write_cmds;
   mongoc_collection_t *collection;
   mongoc_bulk_operation_t *bulk;
   bson_t reply;
   bson_error_t error;
   bool r;

   client = test_framework_client_new ();
   BSON_ASSERT (client);
   has_write_cmds = server_has_write_commands ();

   collection =
      get_test_collection (client, "test_single_error_unordered_bulk");
   BSON_ASSERT (collection);

   create_unique_index (collection);

   bulk = mongoc_collection_create_bulk_operation (collection, false, NULL);

   /* 0 succeeds */
   mongoc_bulk_operation_insert (bulk, tmp_bson ("{'b': 1, 'a': 1}"));
   /* 1 fails */
   mongoc_bulk_operation_update (
      bulk, tmp_bson ("{'b': 2}"), tmp_bson ("{'$set': {'a': 1}}"), true);
   /* 2 succeeds */
   mongoc_bulk_operation_insert (bulk, tmp_bson ("{'b': 3, 'a': 2}"));
   r = (bool) mongoc_bulk_operation_execute (bulk, &reply, &error);

   BSON_ASSERT (!r);
   ASSERT_CMPINT (error.domain, ==, MONGOC_ERROR_COMMAND);
   BSON_ASSERT (error.code);

   /* TODO: CDRIVER-651, BSON_ASSERT contents of the 'op' field */
   ASSERT_MATCH (&reply,
                 "{'nInserted': 2,"
                 " 'nMatched':  0,"
                 " 'nRemoved':  0,"
                 " 'nUpserted': 0,"
                 " 'writeErrors': [{'index': 1,"
                 "                  'code': {'$exists': true},"
                 "                  'errmsg': {'$exists': true}}]}");
   assert_error_count (1, &reply);
   check_n_modified (has_write_cmds, &reply, 0);
   ASSERT_COUNT (2, collection);

   bson_destroy (&reply);
   mongoc_bulk_operation_destroy (bulk);
   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
}


static void
_test_write_concern (bool has_write_commands, bool ordered, bool multi_err)
{
   mock_server_t *mock_server;
   mongoc_client_t *client;
   mongoc_collection_t *collection;
   mongoc_write_concern_t *wc;
   mongoc_bulk_operation_t *bulk;
   bson_t reply;
   bson_error_t error;
   mongoc_insert_flags_t insert_flags;
   future_t *future;
   request_t *request;
   int32_t first_err;
   int32_t second_err;

   /* set wire protocol version for legacy writes or write commands */
   mock_server = mock_server_with_autoismaster (has_write_commands ? 3 : 0);
   mock_server_run (mock_server);
   client = mongoc_client_new_from_uri (mock_server_get_uri (mock_server));
   collection = mongoc_client_get_collection (client, "test", "test");
   wc = mongoc_write_concern_new ();
   mongoc_write_concern_set_w (wc, 2);
   mongoc_write_concern_set_wtimeout (wc, 100);
   bulk = mongoc_collection_create_bulk_operation (collection, ordered, wc);
   mongoc_bulk_operation_insert (bulk, tmp_bson ("{'_id': 1}"));
   mongoc_bulk_operation_remove (bulk, tmp_bson ("{'_id': 2}"));

   future = future_bulk_operation_execute (bulk, &reply, &error);

   if (has_write_commands) {
      request = mock_server_receives_command (
         mock_server,
         "test",
         MONGOC_QUERY_NONE,
         "{'insert': 'test',"
         " 'writeConcern': {'w': 2, 'wtimeout': 100},"
         " 'ordered': %s,"
         " 'documents': [{'_id': 1}]}",
         ordered ? "true" : "false");

      BSON_ASSERT (request);
      mock_server_replies_simple (
         request,
         "{'ok': 1.0, 'n': 1, "
         " 'writeConcernError': {'code': 17, 'errmsg': 'foo'}}");

      request_destroy (request);
      request = mock_server_receives_command (
         mock_server,
         "test",
         MONGOC_QUERY_NONE,
         "{'delete': 'test',"
         " 'writeConcern': {'w': 2, 'wtimeout': 100},"
         " 'ordered': %s,"
         " 'deletes': [{'q': {'_id': 2}, 'limit': 0}]}",
         ordered ? "true" : "false");

      if (multi_err) {
         mock_server_replies_simple (
            request,
            "{'ok': 1.0, 'n': 1, "
            " 'writeConcernError': {'code': 42, 'errmsg': 'bar'}}");
      } else {
         mock_server_replies_simple (request, "{'ok': 1.0, 'n': 1}");
      }

      request_destroy (request);

      /* server fictionally returns 17 and 42; expect driver to use first one */
      first_err = 17;
      second_err = 42;
   } else {
      insert_flags =
         ordered ? MONGOC_INSERT_NONE : MONGOC_INSERT_CONTINUE_ON_ERROR;

      request = mock_server_receives_insert (
         mock_server, "test.test", insert_flags, "{'_id': 1}");

      request_destroy (request);
      request = mock_server_receives_command (
         mock_server,
         "test",
         MONGOC_QUERY_NONE,
         "{'getLastError': 1, 'w': 2, 'wtimeout': 100}");

      BSON_ASSERT (request);
      mock_server_replies_simple (
         request, "{'ok': 1.0, 'n': 1, 'err': 'foo', 'wtimeout': true}");

      request_destroy (request);
      request = mock_server_receives_delete (
         mock_server, "test.test", MONGOC_REMOVE_NONE, "{'_id': 2}");

      request_destroy (request);
      request = mock_server_receives_command (
         mock_server,
         "test",
         MONGOC_QUERY_NONE,
         "{'getLastError': 1, 'w': 2, 'wtimeout': 100}");

      if (multi_err) {
         mock_server_replies_simple (
            request, "{'ok': 1.0, 'n': 1, 'err': 'bar', 'wtimeout': true}");
      } else {
         mock_server_replies_simple (request, "{'ok': 1.0, 'n': 1}");
      }

      request_destroy (request);

      /* The client makes up the error code for legacy writes */
      first_err = second_err = 64;
   }

   /* join thread, BSON_ASSERT mongoc_bulk_operation_execute () returned 0 */
   BSON_ASSERT (!future_get_uint32_t (future));

   if (multi_err) {
      ASSERT_MATCH (&reply,
                    "{'nInserted': 1,"
                    " 'nMatched': 0,"
                    " 'nRemoved': 1,"
                    " 'nUpserted': 0,"
                    " 'writeErrors': [],"
                    " 'writeConcernErrors': ["
                    "     {'code': %d, 'errmsg': 'foo'},"
                    "     {'code': %d, 'errmsg': 'bar'}]}",
                    first_err,
                    second_err);

      ASSERT_CMPSTR ("Multiple write concern errors: \"foo\", \"bar\"",
                     error.message);
   } else {
      ASSERT_MATCH (&reply,
                    "{'nInserted': 1,"
                    " 'nMatched': 0,"
                    " 'nRemoved': 1,"
                    " 'nUpserted': 0,"
                    " 'writeErrors': [],"
                    " 'writeConcernErrors': ["
                    "     {'code': %d, 'errmsg': 'foo'}]}",
                    first_err);
      ASSERT_CMPSTR ("foo", error.message);
   }

   check_n_modified (has_write_commands, &reply, 0);

   ASSERT_CMPINT (MONGOC_ERROR_WRITE_CONCERN, ==, error.domain);
   ASSERT_CMPINT (first_err, ==, error.code);

   future_destroy (future);
   bson_destroy (&reply);
   mongoc_bulk_operation_destroy (bulk);
   mongoc_write_concern_destroy (wc);
   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
   mock_server_destroy (mock_server);
}

static void
test_write_concern_legacy_ordered (void)
{
   _test_write_concern (false, true, false);
}


static void
test_write_concern_legacy_ordered_multi_err (void)
{
   _test_write_concern (false, true, true);
}


static void
test_write_concern_legacy_unordered (void)
{
   _test_write_concern (false, false, false);
}


static void
test_write_concern_legacy_unordered_multi_err (void)
{
   _test_write_concern (false, false, true);
}


static void
test_write_concern_write_command_ordered (void)
{
   _test_write_concern (true, true, false);
}


static void
test_write_concern_write_command_ordered_multi_err (void)
{
   _test_write_concern (true, true, true);
}


static void
test_write_concern_write_command_unordered (void)
{
   _test_write_concern (true, false, false);
}


static void
test_write_concern_write_command_unordered_multi_err (void)
{
   _test_write_concern (true, false, true);
}


static void
_test_write_concern_err_api (bool has_write_commands, int32_t error_api_version)
{
   mock_server_t *mock_server;
   mongoc_client_t *client;
   mongoc_collection_t *collection;
   mongoc_bulk_operation_t *bulk;
   bson_t reply;
   bson_error_t error;
   future_t *future;
   request_t *request;
   uint32_t expected_code;

   /* set wire protocol version for legacy writes or write commands */
   mock_server = mock_server_with_autoismaster (has_write_commands ? 3 : 0);
   mock_server_run (mock_server);
   client = mongoc_client_new_from_uri (mock_server_get_uri (mock_server));
   ASSERT (mongoc_client_set_error_api (client, error_api_version));
   collection = mongoc_client_get_collection (client, "test", "test");
   bulk = mongoc_collection_create_bulk_operation (collection, true, NULL);
   mongoc_bulk_operation_insert (bulk, tmp_bson ("{'_id': 1}"));

   future = future_bulk_operation_execute (bulk, &reply, &error);

   if (has_write_commands) {
      request = mock_server_receives_command (
         mock_server, "test", MONGOC_QUERY_NONE, NULL);

      mock_server_replies_simple (
         request,
         "{'ok': 1.0, 'n': 1, "
         " 'writeConcernError': {'code': 42, 'errmsg': 'foo'}}");
   } else {
      request = mock_server_receives_insert (
         mock_server, "test.test", MONGOC_INSERT_NONE, "{'_id': 1}");

      request_destroy (request);
      request = mock_server_receives_gle (mock_server, "test");
      mock_server_replies_simple (
         request, "{'ok': 1.0, 'n': 1, 'err': 'foo', 'wtimeout': true}");
   }

   BSON_ASSERT (!future_get_uint32_t (future));
   /* legacy write concern errs have no code from server, driver uses 64 */
   expected_code = has_write_commands ? 42 : 64;
   ASSERT_ERROR_CONTAINS (
      error, MONGOC_ERROR_WRITE_CONCERN, expected_code, "foo");

   request_destroy (request);
   future_destroy (future);
   bson_destroy (&reply);
   mongoc_bulk_operation_destroy (bulk);
   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
   mock_server_destroy (mock_server);
}


static void
test_write_concern_error_legacy_v1 (void)
{
   _test_write_concern_err_api (false, 1);
}


static void
test_write_concern_error_write_command_v1 (void)
{
   _test_write_concern_err_api (true, 1);
}


static void
test_write_concern_error_legacy_v2 (void)
{
   _test_write_concern_err_api (false, 2);
}


static void
test_write_concern_error_write_command_v2 (void)
{
   _test_write_concern_err_api (true, 2);
}


static void
test_multiple_error_unordered_bulk (void)
{
   mongoc_client_t *client;
   bool has_write_cmds;
   mongoc_collection_t *collection;
   mongoc_bulk_operation_t *bulk;
   bson_t reply;
   bson_error_t error;
   bool r;

   client = test_framework_client_new ();
   BSON_ASSERT (client);
   has_write_cmds = server_has_write_commands ();

   collection =
      get_test_collection (client, "test_multiple_error_unordered_bulk");
   BSON_ASSERT (collection);

   create_unique_index (collection);

   bulk = mongoc_collection_create_bulk_operation (collection, false, NULL);
   mongoc_bulk_operation_insert (bulk, tmp_bson ("{'b': 1, 'a': 1}"));
   mongoc_bulk_operation_update (
      bulk, tmp_bson ("{'b': 2}"), tmp_bson ("{'$set': {'a': 3}}"), true);
   mongoc_bulk_operation_update (
      bulk, tmp_bson ("{'b': 3}"), tmp_bson ("{'$set': {'a': 4}}"), true);
   mongoc_bulk_operation_update (
      bulk, tmp_bson ("{'b': 4}"), tmp_bson ("{'$set': {'a': 3}}"), true);
   mongoc_bulk_operation_insert (bulk, tmp_bson ("{'b': 5, 'a': 2}"));
   mongoc_bulk_operation_insert (bulk, tmp_bson ("{'b': 6, 'a': 1}"));
   r = (bool) mongoc_bulk_operation_execute (bulk, &reply, &error);

   BSON_ASSERT (!r);
   ASSERT_CMPINT (error.domain, ==, MONGOC_ERROR_COMMAND);
   BSON_ASSERT (error.code);

   /* Assume the update at index 1 runs before the update at index 3,
    * although the spec does not require it. Same for inserts.
    */
   /* TODO: CDRIVER-651, BSON_ASSERT contents of the 'op' field */
   ASSERT_MATCH (&reply,
                 "{'nInserted': 2,"
                 " 'nMatched': 0,"
                 " 'nRemoved': 0,"
                 " 'nUpserted': 2,"
                 /* " 'writeErrors.0.op': {'q': {'b': 4}, 'u': {'$set': {'a':
                    3}}, 'multi': false, 'upsert': true}}," */
                 " 'writeErrors.0.index':  3,"
                 " 'writeErrors.0.code':   {'$exists': true},"
                 " 'writeErrors.1.index':  5,"
                 /* " 'writeErrors.1.op': {'_id': '...', 'b': 6, 'a': 1}," */
                 " 'writeErrors.1.code':   {'$exists': true},"
                 " 'writeErrors.1.errmsg': {'$exists': true}}");
   assert_error_count (2, &reply);
   check_n_modified (has_write_cmds, &reply, 0);

   /*
    * assume the update at index 1 runs before the update at index 3,
    * although the spec does not require it. Same for inserts.
    */
   ASSERT_COUNT (4, collection);

   bson_destroy (&reply);
   mongoc_bulk_operation_destroy (bulk);
   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
}


static void
_test_wtimeout_plus_duplicate_key_err (bool has_write_commands)
{
   mock_server_t *mock_server;
   mongoc_client_t *client;
   mongoc_collection_t *collection;
   mongoc_bulk_operation_t *bulk;
   bson_t reply;
   bson_error_t error;
   future_t *future;
   request_t *request;

   /* set wire protocol version for legacy writes or write commands */
   mock_server = mock_server_with_autoismaster (has_write_commands ? 3 : 0);
   mock_server_run (mock_server);
   client = mongoc_client_new_from_uri (mock_server_get_uri (mock_server));
   collection = mongoc_client_get_collection (client, "test", "test");

   /* unordered bulk */
   bulk = mongoc_collection_create_bulk_operation (collection, false, NULL);
   mongoc_bulk_operation_insert (bulk, tmp_bson ("{'_id': 1}"));
   mongoc_bulk_operation_insert (bulk, tmp_bson ("{'_id': 2}"));
   mongoc_bulk_operation_remove (bulk, tmp_bson ("{'_id': 3}"));
   future = future_bulk_operation_execute (bulk, &reply, &error);

   if (has_write_commands) {
      request = mock_server_receives_command (
         mock_server,
         "test",
         MONGOC_QUERY_NONE,
         "{'insert': 'test',"
         " 'writeConcern': {},"
         " 'ordered': false,"
         " 'documents': [{'_id': 1}, {'_id': 2}]}");

      BSON_ASSERT (request);
      mock_server_replies (
         request,
         0,
         0,
         0,
         1,
         "{'ok': 1.0, 'n': 1,"
         " 'writeErrors': [{'index': 0, 'code': 11000, 'errmsg': 'dupe'}],"
         " 'writeConcernError': {'code': 17, 'errmsg': 'foo'}}");

      request_destroy (request);
      request = mock_server_receives_command (
         mock_server,
         "test",
         MONGOC_QUERY_NONE,
         "{'delete': 'test',"
         " 'writeConcern': {},"
         " 'ordered': false,"
         " 'deletes': [{'q': {'_id': 3}, 'limit': 0}]}");

      BSON_ASSERT (request);
      mock_server_replies (
         request,
         0,
         0,
         0,
         1,
         "{'ok': 1.0, 'n': 1,"
         " 'writeConcernError': {'code': 42, 'errmsg': 'bar'}}");
      request_destroy (request);
   } else {
      request = mock_server_receives_insert (mock_server,
                                             "test.test",
                                             MONGOC_INSERT_CONTINUE_ON_ERROR,
                                             "{'_id': 1}");

      request_destroy (request);
      request = mock_server_receives_gle (mock_server, "test");
      mock_server_replies (request,
                           0,
                           0,
                           0,
                           1,
                           "{'ok': 1.0, 'n': 0, 'code': 11000, 'err': 'dupe'}");

      request_destroy (request);
      request = mock_server_receives_insert (mock_server,
                                             "test.test",
                                             MONGOC_INSERT_CONTINUE_ON_ERROR,
                                             "{'_id': 2}");

      request_destroy (request);
      request = mock_server_receives_gle (mock_server, "test");
      mock_server_replies (
         request,
         0,
         0,
         0,
         1,
         "{'ok': 1.0, 'n': 1, 'err': 'foo', 'wtimeout': true}");

      request_destroy (request);
      request = mock_server_receives_delete (
         mock_server, "test.test", MONGOC_REMOVE_NONE, "{'_id': 3}");

      request_destroy (request);
      request = mock_server_receives_gle (mock_server, "test");
      mock_server_replies (
         request,
         0,
         0,
         0,
         1,
         "{'ok': 1.0, 'n': 1, 'err': 'bar', 'wtimeout': true}");
      request_destroy (request);
   }

   /* mongoc_bulk_operation_execute () returned 0 */
   BSON_ASSERT (!future_get_uint32_t (future));

   /* get err code from server with write commands, otherwise use 64 */
   ASSERT_MATCH (&reply,
                 "{'nInserted': 1,"
                 " 'nMatched': 0,"
                 " 'nRemoved': 1,"
                 " 'nUpserted': 0,"
                 " 'writeErrors': ["
                 "    {'index': 0, 'code': 11000, 'errmsg': 'dupe'}],"
                 " 'writeConcernErrors': ["
                 "    {'code': %d, 'errmsg': 'foo'},"
                 "    {'code': %d, 'errmsg': 'bar'}]}",
                 has_write_commands ? 17 : 64,
                 has_write_commands ? 42 : 64);

   check_n_modified (has_write_commands, &reply, 0);

   future_destroy (future);
   bson_destroy (&reply);
   mongoc_bulk_operation_destroy (bulk);
   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
   mock_server_destroy (mock_server);
}


static void
test_wtimeout_plus_duplicate_key_err_legacy (void)
{
   _test_wtimeout_plus_duplicate_key_err (false);
}


static void
test_wtimeout_plus_duplicate_key_err_write_commands (void)
{
   _test_wtimeout_plus_duplicate_key_err (true);
}


static void
test_large_inserts_ordered (void *ctx)
{
   mongoc_client_t *client;
   bool has_write_cmds;
   bson_t *huge_doc;
   mongoc_collection_t *collection;
   mongoc_bulk_operation_t *bulk;
   bson_t reply;
   bson_error_t error;
   bool r;
   bson_t *big_doc;
   bson_iter_t iter;
   int i;
   const bson_t *retdoc;
   bson_t query = BSON_INITIALIZER;
   mongoc_cursor_t *cursor;

   client = test_framework_client_new ();
   BSON_ASSERT (client);
   has_write_cmds = server_has_write_commands ();

   huge_doc = BCON_NEW ("a", BCON_INT32 (1));
   bson_append_utf8 (huge_doc,
                     "long-key-to-make-this-fail",
                     -1,
                     huge_string (client),
                     (int) huge_string_length (client));

   collection = get_test_collection (client, "test_large_inserts_ordered");
   BSON_ASSERT (collection);

   bulk = mongoc_collection_create_bulk_operation (collection, true, NULL);
   BSON_ASSERT (bulk);
   mongoc_bulk_operation_insert (bulk, tmp_bson ("{'b': 1, 'a': 1}"));
   mongoc_bulk_operation_insert (bulk, huge_doc);
   mongoc_bulk_operation_insert (bulk, tmp_bson ("{'b': 2, 'a': 2}"));

   r = (bool) mongoc_bulk_operation_execute (bulk, &reply, &error);
   BSON_ASSERT (!r);
   ASSERT_CMPINT (error.domain, ==, MONGOC_ERROR_COMMAND);
   ASSERT_MATCH (&reply,
                 "{'nInserted': 1,"
                 " 'nMatched': 0,"
                 " 'nRemoved': 0,"
                 " 'nUpserted': 0,"
                 " 'writeErrors': [{'index':  1}]}");
   assert_error_count (1, &reply);
   check_n_modified (has_write_cmds, &reply, 0);
   ASSERT_COUNT (1, collection);

   cursor = mongoc_collection_find_with_opts (collection, &query, NULL, NULL);
   ASSERT_CURSOR_NEXT (cursor, &retdoc);
   ASSERT_CURSOR_DONE (cursor);

   bson_destroy (&query);
   mongoc_collection_remove (
      collection, MONGOC_REMOVE_NONE, tmp_bson ("{}"), NULL, NULL);

   bson_destroy (&reply);
   mongoc_bulk_operation_destroy (bulk);
   bulk = mongoc_collection_create_bulk_operation (collection, true, NULL);
   BSON_ASSERT (bulk);

   big_doc = tmp_bson ("{'a': 1}");
   bson_append_utf8 (big_doc, "big", -1, four_mb_string (), FOUR_MB);
   bson_iter_init_find (&iter, big_doc, "a");

   for (i = 1; i <= 6; i++) {
      bson_iter_overwrite_int32 (&iter, i);
      mongoc_bulk_operation_insert (bulk, big_doc);
   }

   ASSERT_OR_PRINT ((bool) mongoc_bulk_operation_execute (bulk, &reply, &error),
                    error);
   assert_n_inserted (6, &reply);
   ASSERT_COUNT (6, collection);

   bson_destroy (&reply);
   mongoc_bulk_operation_destroy (bulk);
   mongoc_cursor_destroy (cursor);
   mongoc_collection_destroy (collection);
   bson_destroy (huge_doc);
   mongoc_client_destroy (client);
}


static void
test_large_inserts_unordered (void *ctx)
{
   mongoc_client_t *client;
   bson_t *huge_doc;
   mongoc_collection_t *collection;
   mongoc_bulk_operation_t *bulk;
   bson_t reply;
   bson_error_t error;
   bool r;
   bson_t *big_doc;
   bson_iter_t iter;
   int i;
   const bson_t *retdoc;
   bson_t query = BSON_INITIALIZER;
   mongoc_cursor_t *cursor;

   client = test_framework_client_new ();
   BSON_ASSERT (client);

   huge_doc = BCON_NEW ("a", BCON_INT32 (1));
   bson_append_utf8 (huge_doc,
                     "long-key-to-make-this-fail",
                     -1,
                     huge_string (client),
                     (int) huge_string_length (client));

   collection = get_test_collection (client, "test_large_inserts_unordered");
   BSON_ASSERT (collection);

   bulk = mongoc_collection_create_bulk_operation (collection, false, NULL);
   BSON_ASSERT (bulk);
   mongoc_bulk_operation_insert (bulk, tmp_bson ("{'b': 1, 'a': 1}"));

   /* 1 fails */
   mongoc_bulk_operation_insert (bulk, huge_doc);
   mongoc_bulk_operation_insert (bulk, tmp_bson ("{'b': 2, 'a': 2}"));

   r = (bool) mongoc_bulk_operation_execute (bulk, &reply, &error);
   BSON_ASSERT (!r);
   ASSERT_CMPINT (error.domain, ==, MONGOC_ERROR_COMMAND);
   ASSERT_MATCH (&reply,
                 "{'nInserted': 2,"
                 " 'nMatched':  0,"
                 " 'nRemoved':  0,"
                 " 'nUpserted': 0,"
                 " 'writeErrors': [{"
                 "    'index':  1,"
                 "    'code':   {'$exists': true},"
                 "    'errmsg': {'$exists': true}"
                 " }]}");

   ASSERT_COUNT (2, collection);

   cursor = mongoc_collection_find_with_opts (collection, &query, NULL, NULL);
   ASSERT_CURSOR_NEXT (cursor, &retdoc);
   ASSERT_CURSOR_NEXT (cursor, &retdoc);
   ASSERT_CURSOR_DONE (cursor);

   bson_destroy (&query);
   mongoc_collection_remove (
      collection, MONGOC_REMOVE_NONE, tmp_bson ("{}"), NULL, NULL);

   bson_destroy (&reply);
   mongoc_bulk_operation_destroy (bulk);
   bulk = mongoc_collection_create_bulk_operation (collection, false, NULL);
   BSON_ASSERT (bulk);

   big_doc = tmp_bson ("{'a': 1}");
   bson_append_utf8 (big_doc, "big", -1, four_mb_string (), (int) FOUR_MB);
   bson_iter_init_find (&iter, big_doc, "a");

   for (i = 1; i <= 6; i++) {
      bson_iter_overwrite_int32 (&iter, i);
      mongoc_bulk_operation_insert (bulk, big_doc);
   }

   ASSERT_OR_PRINT ((bool) mongoc_bulk_operation_execute (bulk, &reply, &error),
                    error);
   assert_n_inserted (6, &reply);
   ASSERT_COUNT (6, collection);

   bson_destroy (huge_doc);
   bson_destroy (&reply);
   mongoc_cursor_destroy (cursor);
   mongoc_bulk_operation_destroy (bulk);
   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
}


static void
_test_numerous (bool ordered)
{
   mongoc_client_t *client;
   mongoc_collection_t *collection;
   mongoc_bulk_operation_t *bulk;
   bson_t reply;
   bson_error_t error;
   int n_docs = 4100; /* exceeds max write batch size of 1000 */
   bson_t doc;
   bson_iter_t iter;
   int i;

   client = test_framework_client_new ();
   BSON_ASSERT (client);

   collection = get_test_collection (client, "test_numerous_inserts");
   BSON_ASSERT (collection);

   bulk = mongoc_collection_create_bulk_operation (collection, ordered, NULL);

   /* insert docs {_id: 0} through {_id: n_docs-1} */
   bson_init (&doc);
   BSON_APPEND_INT32 (&doc, "_id", 0);
   bson_iter_init_find (&iter, &doc, "_id");

   for (i = 0; i < n_docs; i++) {
      bson_iter_overwrite_int32 (&iter, i);
      mongoc_bulk_operation_insert (bulk, &doc);
   }

   ASSERT_OR_PRINT ((bool) mongoc_bulk_operation_execute (bulk, &reply, &error),
                    error);

   assert_n_inserted (n_docs, &reply);
   ASSERT_COUNT (n_docs, collection);

   bson_destroy (&reply);
   mongoc_bulk_operation_destroy (bulk);
   bulk = mongoc_collection_create_bulk_operation (collection, ordered, NULL);

   /* use remove_one for docs {_id: 0}, {_id: 2}, ..., {_id: n_docs-2} */
   for (i = 0; i < n_docs; i += 2) {
      bson_iter_overwrite_int32 (&iter, i);
      mongoc_bulk_operation_remove_one (bulk, &doc);
   }

   ASSERT_OR_PRINT ((bool) mongoc_bulk_operation_execute (bulk, &reply, &error),
                    error);

   assert_n_removed (n_docs / 2, &reply);
   ASSERT_COUNT (n_docs / 2, collection);

   bson_destroy (&reply);
   mongoc_bulk_operation_destroy (bulk);
   bulk = mongoc_collection_create_bulk_operation (collection, ordered, NULL);

   /* use remove for docs {_id: 1}, {_id: 3}, ..., {_id: n_docs-1} */
   for (i = 1; i < n_docs; i += 2) {
      bson_iter_overwrite_int32 (&iter, i);
      mongoc_bulk_operation_remove (bulk, &doc);
   }

   ASSERT_OR_PRINT ((bool) mongoc_bulk_operation_execute (bulk, &reply, &error),
                    error);

   assert_n_removed (n_docs / 2, &reply);
   ASSERT_COUNT (0, collection);

   bson_destroy (&reply);
   mongoc_bulk_operation_destroy (bulk);
   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
}


static void
test_numerous_ordered (void)
{
   _test_numerous (true);
}


static void
test_numerous_unordered (void)
{
   _test_numerous (false);
}


static void
test_bulk_edge_over_1000 (void)
{
   mongoc_client_t *client;
   mongoc_collection_t *collection;
   mongoc_bulk_operation_t *bulk_op;
   mongoc_write_concern_t *wc = mongoc_write_concern_new ();
   bson_iter_t iter, error_iter, indexnum;
   bson_t doc, result;
   bson_error_t error;
   int i;

   client = test_framework_client_new ();
   BSON_ASSERT (client);

   collection = get_test_collection (client, "OVER_1000");
   BSON_ASSERT (collection);

   mongoc_write_concern_set_w (wc, 1);

   bulk_op = mongoc_collection_create_bulk_operation (collection, false, wc);

   for (i = 0; i < 1010; i += 3) {
      bson_init (&doc);
      bson_append_int32 (&doc, "_id", -1, i);

      mongoc_bulk_operation_insert (bulk_op, &doc);

      bson_destroy (&doc);
   }

   mongoc_bulk_operation_execute (bulk_op, NULL, &error);

   mongoc_bulk_operation_destroy (bulk_op);

   bulk_op = mongoc_collection_create_bulk_operation (collection, false, wc);
   for (i = 0; i < 1010; i++) {
      bson_init (&doc);
      bson_append_int32 (&doc, "_id", -1, i);

      mongoc_bulk_operation_insert (bulk_op, &doc);

      bson_destroy (&doc);
   }

   mongoc_bulk_operation_execute (bulk_op, &result, &error);

   bson_iter_init_find (&iter, &result, "writeErrors");
   BSON_ASSERT (bson_iter_recurse (&iter, &error_iter));
   BSON_ASSERT (bson_iter_next (&error_iter));

   for (i = 0; i < 1010; i += 3) {
      BSON_ASSERT (bson_iter_recurse (&error_iter, &indexnum));
      BSON_ASSERT (bson_iter_find (&indexnum, "index"));
      if (bson_iter_int32 (&indexnum) != i) {
         fprintf (stderr,
                  "index should be %d, but is %d\n",
                  i,
                  bson_iter_int32 (&indexnum));
      }
      BSON_ASSERT (bson_iter_int32 (&indexnum) == i);
      bson_iter_next (&error_iter);
   }

   mongoc_bulk_operation_destroy (bulk_op);
   bson_destroy (&result);

   mongoc_write_concern_destroy (wc);

   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
}


static void
test_bulk_edge_case_372 (bool ordered)
{
   mongoc_client_t *client;
   bool has_write_cmds;
   mongoc_collection_t *collection;
   mongoc_bulk_operation_t *bulk;
   bson_error_t error;
   bson_iter_t iter;
   bson_iter_t citer;
   bson_t *selector;
   bson_t *update;
   bson_t reply;

   client = test_framework_client_new ();
   BSON_ASSERT (client);
   has_write_cmds = server_has_write_commands ();

   collection = get_test_collection (client, "CDRIVER_372");
   BSON_ASSERT (collection);

   bulk = mongoc_collection_create_bulk_operation (collection, ordered, NULL);
   BSON_ASSERT (bulk);

   selector = tmp_bson ("{'_id': 0}");
   update = tmp_bson ("{'$set': {'a': 0}}");
   mongoc_bulk_operation_update_one (bulk, selector, update, true);

   selector = tmp_bson ("{'a': 1}");
   update = tmp_bson ("{'_id': 1}");
   mongoc_bulk_operation_replace_one (bulk, selector, update, true);

   if (has_write_cmds) {
      /* This is just here to make the counts right in all cases. */
      selector = tmp_bson ("{'_id': 2}");
      update = tmp_bson ("{'_id': 2}");
      mongoc_bulk_operation_replace_one (bulk, selector, update, true);
   } else {
      /* This case is only possible in MongoDB versions before 2.6. */
      selector = tmp_bson ("{'_id': 3}");
      update = tmp_bson ("{'_id': 2}");
      mongoc_bulk_operation_replace_one (bulk, selector, update, true);
   }

   ASSERT_OR_PRINT (mongoc_bulk_operation_execute (bulk, &reply, &error),
                    error);

   ASSERT_MATCH (&reply,
                 "{'nInserted': 0,"
                 " 'nMatched':  0,"
                 " 'nRemoved':  0,"
                 " 'nUpserted': 3,"
                 " 'upserted': ["
                 "     {'index': 0, '_id': 0},"
                 "     {'index': 1, '_id': 1},"
                 "     {'index': 2, '_id': 2}"
                 " ],"
                 " 'writeErrors': []}");

   check_n_modified (has_write_cmds, &reply, 0);

   BSON_ASSERT (bson_iter_init_find (&iter, &reply, "upserted") &&
                BSON_ITER_HOLDS_ARRAY (&iter) &&
                bson_iter_recurse (&iter, &citer));

   bson_destroy (&reply);

   mongoc_collection_drop (collection, NULL);

   mongoc_bulk_operation_destroy (bulk);
   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
}


static void
test_bulk_edge_case_372_ordered (void)
{
   test_bulk_edge_case_372 (true);
}


static void
test_bulk_edge_case_372_unordered (void)
{
   test_bulk_edge_case_372 (false);
}


static void
test_bulk_new (void)
{
   mongoc_bulk_operation_t *bulk;
   mongoc_collection_t *collection;
   mongoc_client_t *client;
   bson_error_t error;
   bson_t empty = BSON_INITIALIZER;
   uint32_t r;

   client = test_framework_client_new ();
   BSON_ASSERT (client);

   collection = get_test_collection (client, "bulk_new");
   BSON_ASSERT (collection);

   bulk = mongoc_bulk_operation_new (true);
   mongoc_bulk_operation_destroy (bulk);

   bulk = mongoc_bulk_operation_new (true);

   r = mongoc_bulk_operation_execute (bulk, NULL, &error);
   BSON_ASSERT (!r);
   ASSERT_CMPINT (error.domain, ==, MONGOC_ERROR_COMMAND);
   ASSERT_CMPINT (error.code, ==, MONGOC_ERROR_COMMAND_INVALID_ARG);

   mongoc_bulk_operation_set_database (bulk, "test");
   r = mongoc_bulk_operation_execute (bulk, NULL, &error);
   BSON_ASSERT (!r);
   ASSERT_CMPINT (error.domain, ==, MONGOC_ERROR_COMMAND);
   ASSERT_CMPINT (error.code, ==, MONGOC_ERROR_COMMAND_INVALID_ARG);

   mongoc_bulk_operation_set_collection (bulk, "test");
   r = mongoc_bulk_operation_execute (bulk, NULL, &error);
   BSON_ASSERT (!r);
   ASSERT_CMPINT (error.domain, ==, MONGOC_ERROR_COMMAND);
   ASSERT_CMPINT (error.code, ==, MONGOC_ERROR_COMMAND_INVALID_ARG);

   mongoc_bulk_operation_set_client (bulk, client);
   r = mongoc_bulk_operation_execute (bulk, NULL, &error);
   BSON_ASSERT (!r);
   ASSERT_CMPINT (error.domain, ==, MONGOC_ERROR_COMMAND);
   ASSERT_CMPINT (error.code, ==, MONGOC_ERROR_COMMAND_INVALID_ARG);

   mongoc_bulk_operation_insert (bulk, &empty);
   ASSERT_OR_PRINT (mongoc_bulk_operation_execute (bulk, NULL, &error), error);

   mongoc_bulk_operation_destroy (bulk);

   mongoc_collection_drop (collection, NULL);

   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
}


typedef enum { INSERT, UPDATE, REMOVE, OP_TYPE_LAST } op_type_t;


static const char *
op_type_str (op_type_t op_type)
{
   switch (op_type) {
   case INSERT:
      return "insert";
   case UPDATE:
      return "update";
   case REMOVE:
      return "remove";
   case OP_TYPE_LAST:
   default:
      fprintf (stderr, "Invalid op_type: : %d\n", op_type);
      abort ();
   }
}


typedef enum { HANGUP, SERVER_ERROR, ERR_TYPE_LAST } err_type_t;


static const char *
err_type_str (err_type_t err_type)
{
   if (err_type == HANGUP) {
      return "hangup";
   } else {
      return "server_error";
   }
}


typedef struct {
   op_type_t op_type;
   err_type_t err_type;
   int pooled;
   int32_t err_api_version;
} err_test_t;


static void
_test_legacy_write_err (void *ctx)
{
   err_test_t *err_test;
   mock_server_t *server;
   mongoc_client_pool_t *pool = NULL;
   mongoc_client_t *client;
   mongoc_collection_t *collection;
   mongoc_bulk_operation_t *bulk;
   bson_t *doc = tmp_bson ("{'_id': 1}");
   bson_t reply;
   bson_error_t error;
   future_t *future;
   request_t *request = NULL;

   err_test = (err_test_t *) ctx;

   server = mock_server_with_autoismaster (0); /* wire version = 0 */
   mock_server_run (server);

   if (err_test->pooled) {
      pool = mongoc_client_pool_new (mock_server_get_uri (server));
      BSON_ASSERT (
         mongoc_client_pool_set_error_api (pool, err_test->err_api_version));

      client = mongoc_client_pool_pop (pool);
   } else {
      client = mongoc_client_new_from_uri (mock_server_get_uri (server));
      BSON_ASSERT (
         mongoc_client_set_error_api (client, err_test->err_api_version));
   }

   collection = mongoc_client_get_collection (client, "test", "test");
   bulk = mongoc_collection_create_bulk_operation (collection, true, NULL);

   switch (err_test->op_type) {
   case INSERT:
      mongoc_bulk_operation_insert (bulk, doc);
      break;
   case UPDATE:
      mongoc_bulk_operation_update (
         bulk, doc, tmp_bson ("{'$inc': {'x': 1}}"), false);
      break;
   case REMOVE:
      mongoc_bulk_operation_remove (bulk, doc);
      break;
   case OP_TYPE_LAST:
   default:
      fprintf (stderr, "Invalid op_type: : %d\n", err_test->op_type);
      abort ();
   }

   future = future_bulk_operation_execute (bulk, &reply, &error);

   switch (err_test->op_type) {
   case INSERT:
      request = mock_server_receives_insert (
         server, "test.test", MONGOC_INSERT_NONE, "{'_id': 1}");
      break;
   case UPDATE:
      request = mock_server_receives_update (server,
                                             "test.test",
                                             MONGOC_UPDATE_MULTI_UPDATE,
                                             "{'_id': 1}",
                                             "{'$inc': {'x': 1}}");
      break;
   case REMOVE:
      request = mock_server_receives_delete (
         server, "test.test", MONGOC_REMOVE_NONE, "{'_id': 1}");
      break;
   case OP_TYPE_LAST:
   default:
      fprintf (stderr, "Invalid op_type: : %d\n", err_test->op_type);
      abort ();
   }

   BSON_ASSERT (request);
   request_destroy (request);

   request = mock_server_receives_gle (server, "test");
   BSON_ASSERT (request);

   if (err_test->err_type == HANGUP) {
      capture_logs (true);
      mock_server_hangs_up (request);
   } else {
      /* getlasterror reply has ok: 1, even if reporting an error */
      mock_server_replies_simple (request,
                                  "{'ok': 1, 'err': 'oops', 'code': 42}");
   }

   request_destroy (request);

   /* bulk operation fails */
   BSON_ASSERT (!future_get_uint32_t (future));

   if (err_test->err_type == HANGUP) {
      ASSERT_ERROR_CONTAINS (error,
                             MONGOC_ERROR_STREAM,
                             MONGOC_ERROR_STREAM_SOCKET,
                             "socket error or timeout");
   } else if (err_test->err_api_version == 2) {
      ASSERT_ERROR_CONTAINS (error, MONGOC_ERROR_SERVER, 42, "oops");
   } else {
      /* legacy error API */
      ASSERT_ERROR_CONTAINS (error, MONGOC_ERROR_COMMAND, 42, "oops");
   }

   future_destroy (future);
   mongoc_bulk_operation_destroy (bulk);
   mongoc_collection_destroy (collection);
   bson_destroy (&reply);

   if (err_test->pooled) {
      mongoc_client_pool_push (pool, client);
      mongoc_client_pool_destroy (pool);
   } else {
      mongoc_client_destroy (client);
   }

   mock_server_destroy (server);
}


static void
test_bulk_write_concern_over_1000 (void)
{
   mongoc_client_t *client;
   mongoc_bulk_operation_t *bulk;
   mongoc_write_concern_t *write_concern;
   bson_t doc;
   bson_error_t error;
   uint32_t success;
   int i;
   char *str;
   bson_t reply;
   bson_iter_t iter;
   bool r;

   client = test_framework_client_new ();
   BSON_ASSERT (client);

   write_concern = mongoc_write_concern_new ();
   mongoc_write_concern_set_w (write_concern, 1);
   mongoc_client_set_write_concern (client, write_concern);

   str = gen_collection_name ("bulk_write_concern_over_1000");
   bulk = mongoc_bulk_operation_new (true);
   mongoc_bulk_operation_set_database (bulk, "test");
   mongoc_bulk_operation_set_collection (bulk, str);
   mongoc_write_concern_set_w (write_concern, 0);
   mongoc_bulk_operation_set_write_concern (bulk, write_concern);
   mongoc_bulk_operation_set_client (bulk, client);

   for (i = 0; i < 1010; i += 3) {
      bson_init (&doc);
      bson_append_int32 (&doc, "_id", -1, i);

      mongoc_bulk_operation_insert (bulk, &doc);

      bson_destroy (&doc);
   }

   success = mongoc_bulk_operation_execute (bulk, NULL, &error);
   ASSERT_OR_PRINT (success, error);

   /* wait for bulk insert to complete on this connection */
   r = mongoc_client_command_simple (
      client, "test", tmp_bson ("{'getlasterror': 1}"), NULL, &reply, &error);

   ASSERT_OR_PRINT (r, error);
   if (bson_iter_init_find (&iter, &reply, "err") &&
       BSON_ITER_HOLDS_UTF8 (&iter)) {
      test_error ("%s", bson_iter_utf8 (&iter, NULL));
      abort ();
   }

   bson_destroy (&reply);
   bson_free (str);
   mongoc_bulk_operation_destroy (bulk);
   mongoc_client_destroy (client);
   mongoc_write_concern_destroy (write_concern);
}


static uint32_t
server_id_for_read_mode (mongoc_client_t *client, mongoc_read_mode_t read_mode)
{
   mongoc_read_prefs_t *prefs;
   mongoc_server_description_t *sd;
   bson_error_t error;
   uint32_t server_id;

   prefs = mongoc_read_prefs_new (read_mode);
   sd =
      mongoc_topology_select (client->topology, MONGOC_SS_READ, prefs, &error);

   ASSERT_OR_PRINT (sd, error);
   server_id = sd->id;

   mongoc_server_description_destroy (sd);
   mongoc_read_prefs_destroy (prefs);

   return server_id;
}


static void
_test_bulk_hint (bool pooled, bool has_write_cmds, bool use_primary)
{
   mock_rs_t *rs;
   mongoc_client_pool_t *pool = NULL;
   mongoc_client_t *client;
   mongoc_collection_t *collection;
   mongoc_bulk_operation_t *bulk;
   bool ret;
   uint32_t server_id;
   bson_t reply;
   bson_error_t error;
   future_t *future;
   request_t *request;

   /* primary, 2 secondaries. set wire version for legacy writes / write cmds */
   rs = mock_rs_with_autoismaster (has_write_cmds ? 3 : 0, true, 2, 0);
   mock_rs_run (rs);

   if (pooled) {
      pool = mongoc_client_pool_new (mock_rs_get_uri (rs));
      client = mongoc_client_pool_pop (pool);
   } else {
      client = mongoc_client_new_from_uri (mock_rs_get_uri (rs));
   }

   /* warm up the client so its server_id is valid */
   ret = mongoc_client_command_simple (
      client, "admin", tmp_bson ("{'isMaster': 1}"), NULL, NULL, &error);
   ASSERT_OR_PRINT (ret, error);

   collection = mongoc_client_get_collection (client, "test", "test");
   bulk = mongoc_collection_create_bulk_operation (collection, true, NULL);
   ASSERT_CMPUINT32 ((uint32_t) 0, ==, mongoc_bulk_operation_get_hint (bulk));
   if (use_primary) {
      server_id = server_id_for_read_mode (client, MONGOC_READ_PRIMARY);
   } else {
      server_id = server_id_for_read_mode (client, MONGOC_READ_SECONDARY);
   }

   mongoc_bulk_operation_set_hint (bulk, server_id);
   ASSERT_CMPUINT32 (server_id, ==, mongoc_bulk_operation_get_hint (bulk));
   mongoc_bulk_operation_insert (bulk, tmp_bson ("{'_id': 1}"));
   future = future_bulk_operation_execute (bulk, &reply, &error);

   if (has_write_cmds) {
      request = mock_rs_receives_command (
         rs, "test", MONGOC_QUERY_NONE, "{'insert': 'test'}");

      BSON_ASSERT (request);
      mock_server_replies_simple (request, "{'ok': 1.0, 'n': 1}");
   } else {
      request = mock_rs_receives_insert (
         rs, "test.test", MONGOC_INSERT_NONE, "{'_id': 1}");

      BSON_ASSERT (request);
      request_destroy (request);
      request = mock_rs_receives_command (
         rs, "test", MONGOC_QUERY_NONE, "{'getLastError': 1}");

      BSON_ASSERT (request);
      mock_server_replies_simple (request, "{'ok': 1.0, 'n': 1}");
   }

   if (use_primary) {
      BSON_ASSERT (mock_rs_request_is_to_primary (rs, request));
   } else {
      BSON_ASSERT (mock_rs_request_is_to_secondary (rs, request));
   }

   ASSERT_CMPUINT32 (server_id, ==, future_get_uint32_t (future));

   request_destroy (request);
   future_destroy (future);
   bson_destroy (&reply);
   mongoc_bulk_operation_destroy (bulk);
   mongoc_collection_destroy (collection);

   if (pooled) {
      mongoc_client_pool_push (pool, client);
      mongoc_client_pool_destroy (pool);
   } else {
      mongoc_client_destroy (client);
   }

   mock_rs_destroy (rs);
}

static void
test_hint_single_legacy_secondary (void)
{
   _test_bulk_hint (false, false, false);
}

static void
test_hint_single_legacy_primary (void)
{
   _test_bulk_hint (false, false, true);
}

static void
test_hint_single_command_secondary (void)
{
   _test_bulk_hint (false, true, false);
}

static void
test_hint_single_command_primary (void)
{
   _test_bulk_hint (false, true, true);
}

static void
test_hint_pooled_legacy_secondary (void)
{
   _test_bulk_hint (true, false, false);
}

static void
test_hint_pooled_legacy_primary (void)
{
   _test_bulk_hint (true, false, true);
}

static void
test_hint_pooled_command_secondary (void)
{
   _test_bulk_hint (true, true, false);
}

static void
test_hint_pooled_command_primary (void)
{
   _test_bulk_hint (true, true, true);
}


static void
test_bulk_reply_w0 (void)
{
   mongoc_client_t *client;
   mongoc_collection_t *collection;
   mongoc_write_concern_t *wc;
   mongoc_bulk_operation_t *bulk;
   bson_error_t error;
   bson_t reply;

   client = test_framework_client_new ();
   collection = get_test_collection (client, "test_insert_w0");
   wc = mongoc_write_concern_new ();
   mongoc_write_concern_set_w (wc, 0);
   bulk = mongoc_collection_create_bulk_operation (collection, true, wc);
   mongoc_bulk_operation_insert (bulk, tmp_bson ("{}"));
   mongoc_bulk_operation_update (
      bulk, tmp_bson ("{}"), tmp_bson ("{'$set': {'x': 1}}"), false);
   mongoc_bulk_operation_remove (bulk, tmp_bson ("{}"));

   ASSERT_OR_PRINT (mongoc_bulk_operation_execute (bulk, &reply, &error),
                    error);

   ASSERT (bson_empty (&reply));

   bson_destroy (&reply);
   mongoc_bulk_operation_destroy (bulk);
   mongoc_write_concern_destroy (wc);
   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
}

typedef enum {
   BULK_REMOVE,
   BULK_REMOVE_ONE,
   BULK_REPLACE_ONE,
   BULK_UPDATE,
   BULK_UPDATE_ONE
} bulkop;

static void
_test_bulk_collation (int w, int wire_version, bulkop op)
{
   mock_server_t *mock_server;
   mongoc_client_t *client;
   mongoc_collection_t *collection;
   mongoc_write_concern_t *wc;
   mongoc_bulk_operation_t *bulk;
   bson_t reply;
   bson_error_t error;
   request_t *request;
   future_t *future;
   bson_t *opts;
   const char *expect;

   mock_server = mock_server_with_autoismaster (wire_version);
   mock_server_run (mock_server);

   client = mongoc_client_new_from_uri (mock_server_get_uri (mock_server));
   collection = mongoc_client_get_collection (client, "test", "test");

   wc = mongoc_write_concern_new ();
   mongoc_write_concern_set_w (wc, w);
   mongoc_write_concern_set_wtimeout (wc, 100);

   opts = BCON_NEW ("collation",
                    "{",
                    "locale",
                    BCON_UTF8 ("en_US"),
                    "caseFirst",
                    BCON_UTF8 ("lower"),
                    "}");

   bulk = mongoc_collection_create_bulk_operation (collection, true, wc);
   switch (op) {
   case BULK_REMOVE:
      mongoc_bulk_operation_remove_many_with_opts (
         bulk, tmp_bson ("{'_id': 1}"), opts, &error);
      expect = "{'delete': 'test',"
               " 'writeConcern': {'w': %d, 'wtimeout': 100},"
               " 'ordered': true,"
               " 'deletes': ["
               "    {'q': {'_id': 1}, 'limit': 0, 'collation': { 'locale': "
               "'en_US', 'caseFirst': 'lower'}}"
               " ]"
               "}";
      break;
   case BULK_REMOVE_ONE:
      mongoc_bulk_operation_remove_one_with_opts (
         bulk, tmp_bson ("{'_id': 2}"), opts, &error);
      expect = "{'delete': 'test',"
               " 'writeConcern': {'w': %d, 'wtimeout': 100},"
               " 'ordered': true,"
               " 'deletes': ["
               "    {'q': {'_id': 2}, 'limit': 1, 'collation': { 'locale': "
               "'en_US', 'caseFirst': 'lower'}}"
               " ]"
               "}";
      break;
   case BULK_REPLACE_ONE:
      mongoc_bulk_operation_replace_one_with_opts (
         bulk, tmp_bson ("{'_id': 3}"), tmp_bson ("{'_id': 4}"), opts, &error);
      expect = "{'update': 'test',"
               " 'writeConcern': {'w': %d, 'wtimeout': 100},"
               " 'ordered': true,"
               " 'updates': ["
               "    {'q': {'_id': 3}, 'u':  {'_id': 4}, 'collation': { "
               "'locale': 'en_US', 'caseFirst': 'lower'}, 'multi': false}"
               " ]"
               "}";
      break;
   case BULK_UPDATE:
      mongoc_bulk_operation_update_many_with_opts (
         bulk,
         tmp_bson ("{'_id': 5}"),
         tmp_bson ("{'$set': {'_id': 6}}"),
         opts,
         &error);
      expect =
         "{'update': 'test',"
         " 'writeConcern': {'w': %d, 'wtimeout': 100},"
         " 'ordered': true,"
         " 'updates': ["
         "    {'q': {'_id': 5}, 'u':  { '$set': {'_id': 6} }, 'collation': { "
         "'locale': 'en_US', 'caseFirst': 'lower'}, 'multi': true }"
         " ]"
         "}";
      break;
   case BULK_UPDATE_ONE:
      mongoc_bulk_operation_update_one_with_opts (
         bulk,
         tmp_bson ("{'_id': 7}"),
         tmp_bson ("{'$set': {'_id': 8}}"),
         opts,
         &error);
      expect =
         "{'update': 'test',"
         " 'writeConcern': {'w': %d, 'wtimeout': 100},"
         " 'ordered': true,"
         " 'updates': ["
         "    {'q': {'_id': 7}, 'u':  { '$set': {'_id': 8} }, 'collation': { "
         "'locale': 'en_US', 'caseFirst': 'lower'}, 'multi': false}"
         " ]"
         "}";
      break;
   default:
      BSON_ASSERT (false);
   }

   future = future_bulk_operation_execute (bulk, &reply, &error);

   if (wire_version >= WIRE_VERSION_COLLATION && w) {
      request = mock_server_receives_command (
         mock_server, "test", MONGOC_QUERY_NONE, expect, w);

      mock_server_replies_simple (request, "{'ok': 1.0, 'n': 1}");
      request_destroy (request);
      ASSERT (future_get_uint32_t (future));
      future_destroy (future);
   } else {
      ASSERT (!future_get_uint32_t (future));
      future_destroy (future);
      if (w) {
         ASSERT_ERROR_CONTAINS (
            error,
            MONGOC_ERROR_COMMAND,
            MONGOC_ERROR_PROTOCOL_BAD_WIRE_VERSION,
            "Collation is not supported by the selected server");
      } else {
         ASSERT_ERROR_CONTAINS (
            error,
            MONGOC_ERROR_COMMAND,
            MONGOC_ERROR_COMMAND_INVALID_ARG,
            "Cannot set collation for unacknowledged writes");
      }
   }


   bson_destroy (opts);
   bson_destroy (&reply);
   mongoc_bulk_operation_destroy (bulk);
   mongoc_write_concern_destroy (wc);
   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
   mock_server_destroy (mock_server);
}

static void
_test_bulk_collation_multi (int w, int wire_version)
{
   mock_server_t *mock_server;
   mongoc_write_concern_t *wc;
   mongoc_client_t *client;
   mongoc_collection_t *collection;
   mongoc_bulk_operation_t *bulk;
   bson_t reply;
   bson_error_t error;
   request_t *request;
   future_t *future;
   bson_t *opts;

   mock_server = mock_server_with_autoismaster (wire_version);
   mock_server_run (mock_server);

   client = mongoc_client_new_from_uri (mock_server_get_uri (mock_server));
   collection = mongoc_client_get_collection (client, "test", "test");

   opts = BCON_NEW ("collation",
                    "{",
                    "locale",
                    BCON_UTF8 ("en_US"),
                    "caseFirst",
                    BCON_UTF8 ("lower"),
                    "}");

   wc = mongoc_write_concern_new ();
   mongoc_write_concern_set_w (wc, w);
   mongoc_write_concern_set_wtimeout (wc, 100);

   bulk = mongoc_collection_create_bulk_operation (collection, true, wc);
   mongoc_bulk_operation_remove_many_with_opts (
      bulk, tmp_bson ("{'_id': 1}"), NULL, &error);
   mongoc_bulk_operation_remove_many_with_opts (
      bulk, tmp_bson ("{'_id': 2}"), opts, &error);
   future = future_bulk_operation_execute (bulk, &reply, &error);

   if (wire_version >= WIRE_VERSION_COLLATION && w) {
      request = mock_server_receives_command (mock_server,
                                              "test",
                                              MONGOC_QUERY_NONE,
                                              "{'delete': 'test',"
                                              " 'ordered': true,"
                                              " 'deletes': ["
                                              "    {'q': {'_id': 1}},"
                                              "    {'q': {'_id': 2}, "
                                              "'collation': { 'locale': "
                                              "'en_US', 'caseFirst': 'lower'}}"
                                              " ]"
                                              "}");
      mock_server_replies_simple (request, "{'ok': 1.0, 'n': 1}");
      request_destroy (request);
      ASSERT (future_get_uint32_t (future));
      future_destroy (future);
   } else {
      ASSERT (!future_get_uint32_t (future));
      future_destroy (future);
      if (w) {
         ASSERT_ERROR_CONTAINS (
            error,
            MONGOC_ERROR_COMMAND,
            MONGOC_ERROR_PROTOCOL_BAD_WIRE_VERSION,
            "Collation is not supported by the selected server");
      } else {
         ASSERT_ERROR_CONTAINS (
            error,
            MONGOC_ERROR_COMMAND,
            MONGOC_ERROR_COMMAND_INVALID_ARG,
            "Cannot set collation for unacknowledged writes");
      }
   }


   bson_destroy (opts);
   bson_destroy (&reply);
   mongoc_bulk_operation_destroy (bulk);
   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
   mongoc_write_concern_destroy (wc);
   mock_server_destroy (mock_server);
}

void
test_bulk_collation_multi_w1_wire5 (void)
{
   _test_bulk_collation_multi (1, WIRE_VERSION_COLLATION);
}

void
test_bulk_collation_multi_w0_wire5 (void)
{
   _test_bulk_collation_multi (0, WIRE_VERSION_COLLATION);
}

void
test_bulk_collation_multi_w1_wire4 (void)
{
   _test_bulk_collation_multi (1, WIRE_VERSION_COLLATION - 1);
}

void
test_bulk_collation_multi_w0_wire4 (void)
{
   _test_bulk_collation_multi (0, WIRE_VERSION_COLLATION - 1);
}

void
test_bulk_collation_w1_wire5 (void)
{
   _test_bulk_collation (1, WIRE_VERSION_COLLATION, BULK_REMOVE);
   _test_bulk_collation (1, WIRE_VERSION_COLLATION, BULK_REMOVE_ONE);
   _test_bulk_collation (1, WIRE_VERSION_COLLATION, BULK_REPLACE_ONE);
   _test_bulk_collation (1, WIRE_VERSION_COLLATION, BULK_UPDATE);
   _test_bulk_collation (1, WIRE_VERSION_COLLATION, BULK_UPDATE_ONE);
}

void
test_bulk_collation_w0_wire5 (void)
{
   _test_bulk_collation (0, WIRE_VERSION_COLLATION, BULK_REMOVE);
   _test_bulk_collation (0, WIRE_VERSION_COLLATION, BULK_REMOVE_ONE);
   _test_bulk_collation (0, WIRE_VERSION_COLLATION, BULK_REPLACE_ONE);
   _test_bulk_collation (0, WIRE_VERSION_COLLATION, BULK_UPDATE);
   _test_bulk_collation (0, WIRE_VERSION_COLLATION, BULK_UPDATE_ONE);
}

void
test_bulk_collation_w1_wire4 (void)
{
   _test_bulk_collation (1, WIRE_VERSION_COLLATION - 1, BULK_REMOVE);
   _test_bulk_collation (1, WIRE_VERSION_COLLATION - 1, BULK_REMOVE_ONE);
   _test_bulk_collation (1, WIRE_VERSION_COLLATION - 1, BULK_REPLACE_ONE);
   _test_bulk_collation (1, WIRE_VERSION_COLLATION - 1, BULK_UPDATE);
   _test_bulk_collation (1, WIRE_VERSION_COLLATION - 1, BULK_UPDATE_ONE);
}

void
test_bulk_collation_w0_wire4 (void)
{
   _test_bulk_collation (0, WIRE_VERSION_COLLATION - 1, BULK_REMOVE);
   _test_bulk_collation (0, WIRE_VERSION_COLLATION - 1, BULK_REMOVE_ONE);
   _test_bulk_collation (0, WIRE_VERSION_COLLATION - 1, BULK_REPLACE_ONE);
   _test_bulk_collation (0, WIRE_VERSION_COLLATION - 1, BULK_UPDATE);
   _test_bulk_collation (0, WIRE_VERSION_COLLATION - 1, BULK_UPDATE_ONE);
}

static void
test_bulk_update_one_error_message (void)
{
   mongoc_client_t *client;
   mongoc_collection_t *collection;
   mongoc_bulk_operation_t *bulk;
   bson_error_t error;

   client = mongoc_client_new ("mongodb://server");
   collection = mongoc_client_get_collection (client, "test", "test");

   bulk = mongoc_collection_create_bulk_operation (collection, true, NULL);
   mongoc_bulk_operation_update_many_with_opts (
      bulk,
      tmp_bson ("{'_id': 5}"),
      tmp_bson ("{'set': {'_id': 6}}"),
      NULL,
      &error);

   ASSERT_ERROR_CONTAINS (
      error,
      MONGOC_ERROR_COMMAND,
      MONGOC_ERROR_COMMAND_INVALID_ARG,
      "Invalid key 'set': update only works with $ operators");


   mongoc_bulk_operation_destroy (bulk);
   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
}


void
test_bulk_install (TestSuite *suite)
{
   op_type_t op_type;
   err_type_t err_type;
   int pooled;
   int32_t err_api_version;
   char *name;
   /* (op types) X (err types) X (pooled / single) X (err API versions) */
   static err_test_t err_tests[3 * 2 * 2 * 2];
   err_test_t *err_test = err_tests;

   for (op_type = (op_type_t) 0; op_type < OP_TYPE_LAST; op_type++) {
      for (err_type = (err_type_t) 0; err_type < ERR_TYPE_LAST; err_type++) {
         for (pooled = 0; pooled <= 1; pooled++) {
            for (err_api_version = 1; err_api_version <= 2; err_api_version++) {
               err_test->op_type = op_type;
               err_test->err_type = err_type;
               err_test->pooled = pooled;
               err_test->err_api_version = err_api_version;

               name = bson_strdup_printf ("/BulkOperation/error/%s/%s/%s/v%d",
                                          op_type_str (op_type),
                                          err_type_str (err_type),
                                          pooled ? "pooled" : "single",
                                          err_api_version);

               TestSuite_AddFull (suite,
                                  name,
                                  _test_legacy_write_err,
                                  NULL,
                                  (void *) err_test,
                                  TestSuite_CheckMockServerAllowed);

               err_test++;

               bson_free (name);
            }
         }
      }
   }

   TestSuite_AddLive (suite, "/BulkOperation/basic", test_bulk);
   TestSuite_AddMockServerTest (suite, "/BulkOperation/error", test_bulk_error);
   TestSuite_AddMockServerTest (
      suite, "/BulkOperation/error/unordered", test_bulk_error_unordered);
   TestSuite_AddLive (
      suite, "/BulkOperation/insert_ordered", test_insert_ordered);
   TestSuite_AddLive (
      suite, "/BulkOperation/insert_unordered", test_insert_unordered);
   TestSuite_AddLive (
      suite, "/BulkOperation/insert_check_keys", test_insert_check_keys);
   TestSuite_AddLive (
      suite, "/BulkOperation/update_ordered", test_update_ordered);
   TestSuite_AddLive (
      suite, "/BulkOperation/update_unordered", test_update_unordered);
   TestSuite_AddLive (suite,
                      "/BulkOperation/update_one_check_keys",
                      test_update_one_check_keys);
   TestSuite_AddLive (
      suite, "/BulkOperation/update_check_keys", test_update_check_keys);
   TestSuite_AddLive (suite,
                      "/BulkOperation/update_one_with_opts_check_keys",
                      test_update_one_with_opts_check_keys);
   TestSuite_AddLive (suite,
                      "/BulkOperation/update_many_with_opts_check_keys",
                      test_update_many_with_opts_check_keys);
   TestSuite_AddLive (suite,
                      "/BulkOperation/update_one_invalid_first",
                      test_update_one_invalid_first);
   TestSuite_AddLive (
      suite, "/BulkOperation/update_invalid_first", test_update_invalid_first);
   TestSuite_AddLive (suite,
                      "/BulkOperation/update_one_with_opts_invalid_first",
                      test_update_one_with_opts_invalid_first);
   TestSuite_AddLive (suite,
                      "/BulkOperation/update_many_with_opts_invalid_first",
                      test_update_many_with_opts_invalid_first);
   TestSuite_AddLive (suite,
                      "/BulkOperation/replace_one_invalid_first",
                      test_replace_one_invalid_first);
   TestSuite_AddLive (suite,
                      "/BulkOperation/replace_one_with_opts_invalid_first",
                      test_replace_one_with_opts_invalid_first);
   TestSuite_AddLive (suite,
                      "/BulkOperation/update_one_invalid_second",
                      test_update_one_invalid_second);
   TestSuite_AddLive (suite,
                      "/BulkOperation/update_invalid_second",
                      test_update_invalid_second);
   TestSuite_AddLive (suite,
                      "/BulkOperation/update_one_with_opts_invalid_second",
                      test_update_one_with_opts_invalid_second);
   TestSuite_AddLive (suite,
                      "/BulkOperation/update_many_with_opts_invalid_second",
                      test_update_many_with_opts_invalid_second);
   TestSuite_AddLive (suite,
                      "/BulkOperation/replace_one_invalid_second",
                      test_replace_one_invalid_second);
   TestSuite_AddLive (suite,
                      "/BulkOperation/replace_one_with_opts_invalid_second",
                      test_replace_one_with_opts_invalid_second);
   TestSuite_AddLive (
      suite, "/BulkOperation/insert_invalid_first", test_insert_invalid_first);
   TestSuite_AddLive (suite,
                      "/BulkOperation/insert_invalid_second",
                      test_insert_invalid_second);
   TestSuite_AddLive (suite,
                      "/BulkOperation/insert_with_opts_invalid_first",
                      test_insert_with_opts_invalid_first);
   TestSuite_AddLive (suite,
                      "/BulkOperation/insert_with_opts_invalid_second",
                      test_insert_with_opts_invalid_second);
   TestSuite_AddLive (suite,
                      "/BulkOperation/insert_into_system_indexes",
                      test_insert_into_system_indexes);
   TestSuite_AddLive (suite,
                      "/BulkOperation/remove_one_after_invalid",
                      test_remove_one_after_invalid);
   TestSuite_AddLive (
      suite, "/BulkOperation/remove_after_invalid", test_remove_after_invalid);
   TestSuite_AddLive (suite,
                      "/BulkOperation/remove_one_with_opts_after_invalid",
                      test_remove_one_with_opts_after_invalid);
   TestSuite_AddLive (suite,
                      "/BulkOperation/remove_many_with_opts_after_invalid",
                      test_remove_many_with_opts_after_invalid);
   TestSuite_AddLive (
      suite, "/BulkOperation/upsert_ordered", test_upsert_ordered);
   TestSuite_AddLive (
      suite, "/BulkOperation/upsert_unordered", test_upsert_unordered);
   TestSuite_AddFull (suite,
                      "/BulkOperation/upsert_unordered_oversized",
                      test_upsert_unordered_oversized,
                      NULL,
                      NULL,
                      test_framework_skip_if_slow_or_live);
   TestSuite_AddFull (suite,
                      "/BulkOperation/upsert_large",
                      test_upsert_large,
                      NULL,
                      NULL,
                      test_framework_skip_if_slow_or_live);
   TestSuite_AddFull (suite,
                      "/BulkOperation/upsert_huge",
                      test_upsert_huge,
                      NULL,
                      NULL,
                      test_framework_skip_if_slow_or_live);
   TestSuite_AddLive (suite,
                      "/BulkOperation/upserted_index_ordered",
                      test_upserted_index_ordered);
   TestSuite_AddLive (suite,
                      "/BulkOperation/upserted_index_unordered",
                      test_upserted_index_unordered);
   TestSuite_AddLive (
      suite, "/BulkOperation/update_one_ordered", test_update_one_ordered);
   TestSuite_AddLive (
      suite, "/BulkOperation/update_one_unordered", test_update_one_unordered);
   TestSuite_AddLive (
      suite, "/BulkOperation/replace_one_ordered", test_replace_one_ordered);
   TestSuite_AddLive (suite,
                      "/BulkOperation/replace_one_unordered",
                      test_replace_one_unordered);
   TestSuite_AddLive (
      suite, "/BulkOperation/replace_one/keys", test_replace_one_check_keys);
   TestSuite_AddLive (suite,
                      "/BulkOperation/replace_one_with_opts/keys",
                      test_replace_one_with_opts_check_keys);
   TestSuite_AddLive (suite, "/BulkOperation/index_offset", test_index_offset);
   TestSuite_AddLive (
      suite, "/BulkOperation/single_ordered_bulk", test_single_ordered_bulk);
   TestSuite_AddLive (suite,
                      "/BulkOperation/insert_continue_on_error",
                      test_insert_continue_on_error);
   TestSuite_AddLive (suite,
                      "/BulkOperation/update_continue_on_error",
                      test_update_continue_on_error);
   TestSuite_AddLive (suite,
                      "/BulkOperation/remove_continue_on_error",
                      test_remove_continue_on_error);
   TestSuite_AddLive (suite,
                      "/BulkOperation/single_error_ordered_bulk",
                      test_single_error_ordered_bulk);
   TestSuite_AddLive (suite,
                      "/BulkOperation/multiple_error_ordered_bulk",
                      test_multiple_error_ordered_bulk);
   TestSuite_AddLive (suite,
                      "/BulkOperation/single_unordered_bulk",
                      test_single_unordered_bulk);
   TestSuite_AddLive (suite,
                      "/BulkOperation/single_error_unordered_bulk",
                      test_single_error_unordered_bulk);
   TestSuite_AddMockServerTest (suite,
                                "/BulkOperation/write_concern/legacy/ordered",
                                test_write_concern_legacy_ordered);
   TestSuite_AddMockServerTest (
      suite,
      "/BulkOperation/write_concern/legacy/ordered/multi_err",
      test_write_concern_legacy_ordered_multi_err);
   TestSuite_AddMockServerTest (suite,
                                "/BulkOperation/write_concern/legacy/unordered",
                                test_write_concern_legacy_unordered);
   TestSuite_AddMockServerTest (
      suite,
      "/BulkOperation/write_concern/legacy/unordered/multi_err",
      test_write_concern_legacy_unordered_multi_err);
   TestSuite_AddMockServerTest (
      suite,
      "/BulkOperation/write_concern/write_command/ordered",
      test_write_concern_write_command_ordered);
   TestSuite_AddMockServerTest (
      suite,
      "/BulkOperation/write_concern/write_command/ordered/multi_err",
      test_write_concern_write_command_ordered_multi_err);
   TestSuite_AddMockServerTest (
      suite,
      "/BulkOperation/write_concern/write_command/unordered",
      test_write_concern_write_command_unordered);
   TestSuite_AddMockServerTest (
      suite,
      "/BulkOperation/write_concern/write_command/unordered/multi_err",
      test_write_concern_write_command_unordered_multi_err);
   TestSuite_AddMockServerTest (suite,
                                "/BulkOperation/write_concern/error/legacy/v1",
                                test_write_concern_error_legacy_v1);
   TestSuite_AddMockServerTest (
      suite,
      "/BulkOperation/write_concern/error/write_command/v1",
      test_write_concern_error_write_command_v1);
   TestSuite_AddMockServerTest (suite,
                                "/BulkOperation/write_concern/error/legacy/v2",
                                test_write_concern_error_legacy_v2);
   TestSuite_AddMockServerTest (
      suite,
      "/BulkOperation/write_concern/error/write_command/v2",
      test_write_concern_error_write_command_v2);
   TestSuite_AddLive (suite,
                      "/BulkOperation/multiple_error_unordered_bulk",
                      test_multiple_error_unordered_bulk);
   TestSuite_AddMockServerTest (suite,
                                "/BulkOperation/wtimeout_duplicate_key/legacy",
                                test_wtimeout_plus_duplicate_key_err_legacy);
   TestSuite_AddMockServerTest (
      suite,
      "/BulkOperation/wtimeout_duplicate_key/write_commands",
      test_wtimeout_plus_duplicate_key_err_write_commands);
   TestSuite_AddFull (suite,
                      "/BulkOperation/large_inserts_ordered",
                      test_large_inserts_ordered,
                      NULL,
                      NULL,
                      test_framework_skip_if_slow_or_live);
   TestSuite_AddFull (suite,
                      "/BulkOperation/large_inserts_unordered",
                      test_large_inserts_unordered,
                      NULL,
                      NULL,
                      test_framework_skip_if_slow_or_live);
   TestSuite_AddLive (
      suite, "/BulkOperation/numerous_ordered", test_numerous_ordered);
   TestSuite_AddLive (
      suite, "/BulkOperation/numerous_unordered", test_numerous_unordered);
   TestSuite_AddLive (suite,
                      "/BulkOperation/CDRIVER-372_ordered",
                      test_bulk_edge_case_372_ordered);
   TestSuite_AddLive (suite,
                      "/BulkOperation/CDRIVER-372_unordered",
                      test_bulk_edge_case_372_unordered);
   TestSuite_AddLive (suite, "/BulkOperation/new", test_bulk_new);
   TestSuite_AddLive (
      suite, "/BulkOperation/over_1000", test_bulk_edge_over_1000);
   TestSuite_AddLive (suite,
                      "/BulkOperation/write_concern/over_1000",
                      test_bulk_write_concern_over_1000);
   TestSuite_AddMockServerTest (suite,
                                "/BulkOperation/hint/single/legacy/secondary",
                                test_hint_single_legacy_secondary);
   TestSuite_AddMockServerTest (suite,
                                "/BulkOperation/hint/single/legacy/primary",
                                test_hint_single_legacy_primary);
   TestSuite_AddMockServerTest (suite,
                                "/BulkOperation/hint/single/command/secondary",
                                test_hint_single_command_secondary);
   TestSuite_AddMockServerTest (suite,
                                "/BulkOperation/hint/single/command/primary",
                                test_hint_single_command_primary);
   TestSuite_AddMockServerTest (suite,
                                "/BulkOperation/hint/pooled/legacy/secondary",
                                test_hint_pooled_legacy_secondary);
   TestSuite_AddMockServerTest (suite,
                                "/BulkOperation/hint/pooled/legacy/primary",
                                test_hint_pooled_legacy_primary);
   TestSuite_AddMockServerTest (suite,
                                "/BulkOperation/hint/pooled/command/secondary",
                                test_hint_pooled_command_secondary);
   TestSuite_AddMockServerTest (suite,
                                "/BulkOperation/hint/pooled/command/primary",
                                test_hint_pooled_command_primary);
   TestSuite_AddLive (suite, "/BulkOperation/reply_w0", test_bulk_reply_w0);
   TestSuite_AddMockServerTest (suite,
                                "/BulkOperation/opts/collation/w0/wire5",
                                test_bulk_collation_w0_wire5);
   TestSuite_AddMockServerTest (suite,
                                "/BulkOperation/opts/collation/w0/wire4",
                                test_bulk_collation_w0_wire4);
   TestSuite_AddMockServerTest (suite,
                                "/BulkOperation/opts/collation/w1/wire5",
                                test_bulk_collation_w1_wire5);
   TestSuite_AddMockServerTest (suite,
                                "/BulkOperation/opts/collation/w1/wire4",
                                test_bulk_collation_w1_wire4);
   TestSuite_AddMockServerTest (suite,
                                "/BulkOperation/opts/collation/multi/w0/wire5",
                                test_bulk_collation_multi_w0_wire5);
   TestSuite_AddMockServerTest (suite,
                                "/BulkOperation/opts/collation/multi/w0/wire4",
                                test_bulk_collation_multi_w0_wire4);
   TestSuite_AddMockServerTest (suite,
                                "/BulkOperation/opts/collation/multi/w1/wire5",
                                test_bulk_collation_multi_w1_wire5);
   TestSuite_AddMockServerTest (suite,
                                "/BulkOperation/opts/collation/multi/w1/wire4",
                                test_bulk_collation_multi_w1_wire4);
   TestSuite_Add (suite,
                  "/BulkOperation/update_one/error_message",
                  test_bulk_update_one_error_message);
}
