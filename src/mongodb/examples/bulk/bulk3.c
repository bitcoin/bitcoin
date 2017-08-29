#include <assert.h>
#include <bcon.h>
#include <mongoc.h>
#include <stdio.h>

static void
bulk3 (mongoc_collection_t *collection)
{
   mongoc_bulk_operation_t *bulk;
   bson_error_t error;
   bson_t *query;
   bson_t *doc;
   bson_t reply;
   char *str;
   bool ret;

   /* false indicates unordered */
   bulk = mongoc_collection_create_bulk_operation (collection, false, NULL);

   /* Add a document */
   doc = BCON_NEW ("_id", BCON_INT32 (1));
   mongoc_bulk_operation_insert (bulk, doc);
   bson_destroy (doc);

   /* remove {_id: 2} */
   query = BCON_NEW ("_id", BCON_INT32 (2));
   mongoc_bulk_operation_remove_one (bulk, query);
   bson_destroy (query);

   /* insert {_id: 3} */
   doc = BCON_NEW ("_id", BCON_INT32 (3));
   mongoc_bulk_operation_insert (bulk, doc);
   bson_destroy (doc);

   /* replace {_id:4} {'i': 1} */
   query = BCON_NEW ("_id", BCON_INT32 (4));
   doc = BCON_NEW ("i", BCON_INT32 (1));
   mongoc_bulk_operation_replace_one (bulk, query, doc, false);
   bson_destroy (query);
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
   mongoc_client_t *client;
   mongoc_collection_t *collection;

   mongoc_init ();

   client = mongoc_client_new ("mongodb://localhost/?appname=bulk3-example");
   mongoc_client_set_error_api (client, 2);
   collection = mongoc_client_get_collection (client, "test", "test");

   bulk3 (collection);

   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);

   mongoc_cleanup ();

   return 0;
}
