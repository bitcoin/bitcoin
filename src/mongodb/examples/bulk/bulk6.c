#include <bcon.h>
#include <mongoc.h>
#include <stdio.h>

static void
bulk6 (mongoc_collection_t *collection)
{
   mongoc_write_concern_t *wc;
   mongoc_bulk_operation_t *bulk;
   bson_error_t error;
   bson_t *doc;
   bson_t *selector;
   bson_t reply;
   char *str;
   bool ret;

   wc = mongoc_write_concern_new ();
   mongoc_write_concern_set_w (wc, 0);

   bulk = mongoc_collection_create_bulk_operation (collection, true, wc);

   doc = BCON_NEW ("_id", BCON_INT32 (10));
   mongoc_bulk_operation_insert (bulk, doc);
   bson_destroy (doc);

   selector = BCON_NEW ("_id", BCON_INT32 (11));
   mongoc_bulk_operation_remove_one (bulk, selector);
   bson_destroy (selector);

   ret = mongoc_bulk_operation_execute (bulk, &reply, &error);

   str = bson_as_canonical_extended_json (&reply, NULL);
   printf ("%s\n", str);
   bson_free (str);

   if (!ret) {
      printf ("Error: %s\n", error.message);
   }

   bson_destroy (&reply);
   mongoc_bulk_operation_destroy (bulk);
   mongoc_write_concern_destroy (wc);
}

int
main (int argc, char *argv[])
{
   mongoc_client_t *client;
   mongoc_collection_t *collection;

   mongoc_init ();

   client = mongoc_client_new ("mongodb://localhost/?appname=bulk6-example");
   mongoc_client_set_error_api (client, 2);
   collection = mongoc_client_get_collection (client, "test", "test");

   bulk6 (collection);

   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);

   mongoc_cleanup ();

   return 0;
}
