#include <mongoc.h>
#include <stdio.h>

static void
print_pipeline (mongoc_collection_t *collection)
{
   mongoc_cursor_t *cursor;
   bson_error_t error;
   const bson_t *doc;
   bson_t *pipeline;
   char *str;

   pipeline = BCON_NEW ("pipeline",
                        "[",
                        "{",
                        "$group",
                        "{",
                        "_id",
                        "$state",
                        "total_pop",
                        "{",
                        "$sum",
                        "$pop",
                        "}",
                        "}",
                        "}",
                        "{",
                        "$match",
                        "{",
                        "total_pop",
                        "{",
                        "$gte",
                        BCON_INT32 (10000000),
                        "}",
                        "}",
                        "}",
                        "]");

   cursor = mongoc_collection_aggregate (
      collection, MONGOC_QUERY_NONE, pipeline, NULL, NULL);

   while (mongoc_cursor_next (cursor, &doc)) {
      str = bson_as_canonical_extended_json (doc, NULL);
      printf ("%s\n", str);
      bson_free (str);
   }

   if (mongoc_cursor_error (cursor, &error)) {
      fprintf (stderr, "Cursor Failure: %s\n", error.message);
   }

   mongoc_cursor_destroy (cursor);
   bson_destroy (pipeline);
}

int
main (int argc, char *argv[])
{
   mongoc_client_t *client;
   mongoc_collection_t *collection;

   mongoc_init ();

   client = mongoc_client_new (
      "mongodb://localhost:27017?appname=aggregation-example");
   mongoc_client_set_error_api (client, 2);
   collection = mongoc_client_get_collection (client, "test", "zipcodes");

   print_pipeline (collection);

   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);

   mongoc_cleanup ();

   return 0;
}
