#include <assert.h>
#include <bcon.h>
#include <mongoc.h>
#include <stdio.h>

static void
bulk5_fail (mongoc_collection_t *collection)
{
   mongoc_bulk_operation_t *bulk;
   bson_error_t error;
   bson_t *doc;
   bson_t reply;
   char *str;
   bool ret;

   bulk = mongoc_collection_create_bulk_operation (collection, true, NULL);

   /* Two inserts */
   doc = BCON_NEW ("_id", BCON_INT32 (31));
   mongoc_bulk_operation_insert (bulk, doc);
   bson_destroy (doc);

   doc = BCON_NEW ("_id", BCON_INT32 (32));
   mongoc_bulk_operation_insert (bulk, doc);
   bson_destroy (doc);

   /* The above documents do not comply to the schema validation rules
    * we created previously, so this will result in an error */
   ret = mongoc_bulk_operation_execute (bulk, &reply, &error);

   str = bson_as_canonical_extended_json (&reply, NULL);
   printf ("%s\n", str);
   bson_free (str);

   if (!ret) {
      printf ("Error: %s\n", error.message);
   }

   bson_destroy (&reply);
   mongoc_bulk_operation_destroy (bulk);
}

static void
bulk5_success (mongoc_collection_t *collection)
{
   mongoc_bulk_operation_t *bulk;
   bson_error_t error;
   bson_t *doc;
   bson_t reply;
   char *str;
   bool ret;

   bulk = mongoc_collection_create_bulk_operation (collection, true, NULL);

   /* Allow this document to bypass document validation.
    * NOTE: When authentication is enabled, the authenticated user must have
    * either the "dbadmin" or "restore" roles to bypass document validation */
   mongoc_bulk_operation_set_bypass_document_validation (bulk, true);

   /* Two inserts */
   doc = BCON_NEW ("_id", BCON_INT32 (31));
   mongoc_bulk_operation_insert (bulk, doc);
   bson_destroy (doc);

   doc = BCON_NEW ("_id", BCON_INT32 (32));
   mongoc_bulk_operation_insert (bulk, doc);
   bson_destroy (doc);

   ret = mongoc_bulk_operation_execute (bulk, &reply, &error);

   str = bson_as_canonical_extended_json (&reply, NULL);
   printf ("%s\n", str);
   bson_free (str);

   if (!ret) {
      printf ("Error: %s\n", error.message);
   }

   bson_destroy (&reply);
   mongoc_bulk_operation_destroy (bulk);
}

int
main (int argc, char *argv[])
{
   bson_t *options;
   bson_error_t error;
   mongoc_client_t *client;
   mongoc_collection_t *collection;
   mongoc_database_t *database;

   mongoc_init ();

   client = mongoc_client_new ("mongodb://localhost/?appname=bulk5-example");
   mongoc_client_set_error_api (client, 2);
   database = mongoc_client_get_database (client, "testasdf");

   /* Create schema validator */
   options = BCON_NEW (
      "validator", "{", "number", "{", "$gte", BCON_INT32 (5), "}", "}");
   collection =
      mongoc_database_create_collection (database, "collname", options, &error);

   if (collection) {
      bulk5_fail (collection);
      bulk5_success (collection);
      mongoc_collection_destroy (collection);
   } else {
      fprintf (stderr, "Couldn't create collection: '%s'\n", error.message);
   }

   bson_free (options);
   mongoc_database_destroy (database);
   mongoc_client_destroy (client);

   mongoc_cleanup ();

   return 0;
}
