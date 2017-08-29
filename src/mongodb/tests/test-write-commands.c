#include <bcon.h>
#include <mongoc.h>

#include "mongoc-client-private.h"
#include "mongoc-collection-private.h"
#include "mongoc-write-command-private.h"
#include "mongoc-write-concern-private.h"

#include "TestSuite.h"

#include "test-libmongoc.h"
#include "test-conveniences.h"


static void
test_split_insert (void)
{
   mongoc_bulk_write_flags_t write_flags = MONGOC_BULK_WRITE_FLAGS_INIT;
   mongoc_write_command_t command;
   mongoc_write_result_t result;
   mongoc_collection_t *collection;
   mongoc_client_t *client;
   bson_oid_t oid;
   bson_t **docs;
   bson_t reply = BSON_INITIALIZER;
   bson_error_t error;
   mongoc_server_stream_t *server_stream;
   int i;
   bool r;

   client = test_framework_client_new ();
   BSON_ASSERT (client);

   collection = get_test_collection (client, "test_split_insert");
   BSON_ASSERT (collection);

   docs = (bson_t **) bson_malloc (sizeof (bson_t *) * 3000);

   for (i = 0; i < 3000; i++) {
      docs[i] = bson_new ();
      bson_oid_init (&oid, NULL);
      BSON_APPEND_OID (docs[i], "_id", &oid);
   }

   _mongoc_write_result_init (&result);

   _mongoc_write_command_init_insert (
      &command, docs[0], write_flags, ++client->cluster.operation_id, true);

   for (i = 1; i < 3000; i++) {
      _mongoc_write_command_insert_append (&command, docs[i]);
   }

   server_stream = mongoc_cluster_stream_for_writes (&client->cluster, &error);
   ASSERT_OR_PRINT (server_stream, error);
   _mongoc_write_command_execute (&command,
                                  client,
                                  server_stream,
                                  collection->db,
                                  collection->collection,
                                  NULL,
                                  0,
                                  &result);

   r = _mongoc_write_result_complete (&result,
                                      2,
                                      collection->write_concern,
                                      (mongoc_error_domain_t) 0,
                                      &reply,
                                      &error);
   ASSERT_OR_PRINT (r, error);
   BSON_ASSERT (result.nInserted == 3000);

   _mongoc_write_command_destroy (&command);
   _mongoc_write_result_destroy (&result);

   ASSERT_OR_PRINT (mongoc_collection_drop (collection, &error), error);

   for (i = 0; i < 3000; i++) {
      bson_destroy (docs[i]);
   }

   bson_free (docs);
   mongoc_server_stream_cleanup (server_stream);
   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
}


static void
test_invalid_write_concern (void)
{
   mongoc_bulk_write_flags_t write_flags = MONGOC_BULK_WRITE_FLAGS_INIT;
   mongoc_write_command_t command;
   mongoc_write_result_t result;
   mongoc_collection_t *collection;
   mongoc_client_t *client;
   mongoc_write_concern_t *write_concern;
   mongoc_server_stream_t *server_stream;
   bson_t *doc;
   bson_t reply = BSON_INITIALIZER;
   bson_error_t error;
   bool r;

   client = test_framework_client_new ();
   BSON_ASSERT (client);

   collection = get_test_collection (client, "test_invalid_write_concern");
   BSON_ASSERT (collection);

   write_concern = mongoc_write_concern_new ();
   BSON_ASSERT (write_concern);
   mongoc_write_concern_set_w (write_concern, 0);
   mongoc_write_concern_set_journal (write_concern, true);
   BSON_ASSERT (!mongoc_write_concern_is_valid (write_concern));

   doc = BCON_NEW ("_id", BCON_INT32 (0));

   _mongoc_write_command_init_insert (
      &command, doc, write_flags, ++client->cluster.operation_id, true);
   _mongoc_write_result_init (&result);
   server_stream = mongoc_cluster_stream_for_writes (&client->cluster, &error);
   ASSERT_OR_PRINT (server_stream, error);
   _mongoc_write_command_execute (&command,
                                  client,
                                  server_stream,
                                  collection->db,
                                  collection->collection,
                                  write_concern,
                                  0,
                                  &result);

   r = _mongoc_write_result_complete (&result,
                                      2,
                                      collection->write_concern,
                                      (mongoc_error_domain_t) 0,
                                      &reply,
                                      &error);

   BSON_ASSERT (!r);
   ASSERT_CMPINT (error.domain, ==, MONGOC_ERROR_COMMAND);
   ASSERT_CMPINT (error.code, ==, MONGOC_ERROR_COMMAND_INVALID_ARG);

   _mongoc_write_command_destroy (&command);
   _mongoc_write_result_destroy (&result);

   bson_destroy (doc);
   mongoc_server_stream_cleanup (server_stream);
   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
   mongoc_write_concern_destroy (write_concern);
}

static void
test_bypass_validation (void *context)
{
   mongoc_collection_t *collection2;
   mongoc_collection_t *collection;
   bson_t reply;
   mongoc_bulk_operation_t *bulk;
   mongoc_database_t *database;
   mongoc_write_concern_t *wr;
   mongoc_client_t *client;
   bson_error_t error;
   bson_t *options;
   char *collname;
   char *dbname;
   int r;
   int i;

   client = test_framework_client_new ();
   BSON_ASSERT (client);

   dbname = gen_collection_name ("dbtest");
   collname = gen_collection_name ("bypass");
   database = mongoc_client_get_database (client, dbname);
   collection = mongoc_database_get_collection (database, collname);
   BSON_ASSERT (collection);

   options = tmp_bson (
      "{'validator': {'number': {'$gte': 5}}, 'validationAction': 'error'}");
   collection2 =
      mongoc_database_create_collection (database, collname, options, &error);
   ASSERT_OR_PRINT (collection2, error);
   mongoc_collection_destroy (collection2);

   /* {{{ Default fails validation */
   bulk = mongoc_collection_create_bulk_operation (collection, true, NULL);
   for (i = 0; i < 3; i++) {
      bson_t *doc = tmp_bson ("{'number': 3, 'high': %d }", i);

      mongoc_bulk_operation_insert (bulk, doc);
   }
   r = mongoc_bulk_operation_execute (bulk, &reply, &error);
   bson_destroy (&reply);
   ASSERT (!r);

   ASSERT_ERROR_CONTAINS (
      error, MONGOC_ERROR_COMMAND, 121, "Document failed validation");
   mongoc_bulk_operation_destroy (bulk);
   /* }}} */

   /* {{{ bypass_document_validation=false Fails validation */
   bulk = mongoc_collection_create_bulk_operation (collection, true, NULL);
   mongoc_bulk_operation_set_bypass_document_validation (bulk, false);
   for (i = 0; i < 3; i++) {
      bson_t *doc = tmp_bson ("{'number': 3, 'high': %d }", i);

      mongoc_bulk_operation_insert (bulk, doc);
   }
   r = mongoc_bulk_operation_execute (bulk, &reply, &error);
   bson_destroy (&reply);
   ASSERT (!r);

   ASSERT_ERROR_CONTAINS (
      error, MONGOC_ERROR_COMMAND, 121, "Document failed validation");
   mongoc_bulk_operation_destroy (bulk);
   /* }}} */

   /* {{{ bypass_document_validation=true ignores validation */
   bulk = mongoc_collection_create_bulk_operation (collection, true, NULL);
   mongoc_bulk_operation_set_bypass_document_validation (bulk, true);
   for (i = 0; i < 3; i++) {
      bson_t *doc = tmp_bson ("{'number': 3, 'high': %d }", i);

      mongoc_bulk_operation_insert (bulk, doc);
   }
   r = mongoc_bulk_operation_execute (bulk, &reply, &error);
   bson_destroy (&reply);
   ASSERT_OR_PRINT (r, error);
   mongoc_bulk_operation_destroy (bulk);
   /* }}} */

   /* {{{ w=0 and bypass_document_validation=set fails */
   bulk = mongoc_collection_create_bulk_operation (collection, true, NULL);
   wr = mongoc_write_concern_new ();
   mongoc_write_concern_set_w (wr, 0);
   mongoc_bulk_operation_set_write_concern (bulk, wr);
   mongoc_bulk_operation_set_bypass_document_validation (bulk, true);
   for (i = 0; i < 3; i++) {
      bson_t *doc = tmp_bson ("{'number': 3, 'high': %d }", i);

      mongoc_bulk_operation_insert (bulk, doc);
   }
   r = mongoc_bulk_operation_execute (bulk, &reply, &error);
   bson_destroy (&reply);
   ASSERT_OR_PRINT (!r, error);
   ASSERT_ERROR_CONTAINS (
      error,
      MONGOC_ERROR_COMMAND,
      MONGOC_ERROR_COMMAND_INVALID_ARG,
      "Cannot set bypassDocumentValidation for unacknowledged writes");
   mongoc_bulk_operation_destroy (bulk);
   mongoc_write_concern_destroy (wr);
   /* }}} */

   ASSERT_OR_PRINT (mongoc_collection_drop (collection, &error), error);

   bson_free (dbname);
   bson_free (collname);
   mongoc_database_destroy (database);
   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
}

void
test_write_command_install (TestSuite *suite)
{
   TestSuite_AddLive (suite, "/WriteCommand/split_insert", test_split_insert);
   TestSuite_AddLive (
      suite, "/WriteCommand/invalid_write_concern", test_invalid_write_concern);
   TestSuite_AddFull (suite,
                      "/WriteCommand/bypass_validation",
                      test_bypass_validation,
                      NULL,
                      NULL,
                      test_framework_skip_if_max_wire_version_less_than_4);
}
