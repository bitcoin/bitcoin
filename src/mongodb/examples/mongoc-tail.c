#include <bson.h>
#include <mongoc.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
#define sleep(_n) Sleep ((_n) *1000)
#endif


static void
print_bson (const bson_t *b)
{
   char *str;

   str = bson_as_canonical_extended_json (b, NULL);
   fprintf (stdout, "%s\n", str);
   bson_free (str);
}


static mongoc_cursor_t *
query_collection (mongoc_collection_t *collection, uint32_t last_time)
{
   mongoc_cursor_t *cursor;
   bson_t query;
   bson_t gt;
   bson_t opts;

   BSON_ASSERT (collection);

   bson_init (&query);
   BSON_APPEND_DOCUMENT_BEGIN (&query, "ts", &gt);
   BSON_APPEND_TIMESTAMP (&gt, "$gt", last_time, 0);
   bson_append_document_end (&query, &gt);

   bson_init (&opts);
   BSON_APPEND_BOOL (&opts, "tailable", true);
   BSON_APPEND_BOOL (&opts, "awaitData", true);

   cursor = mongoc_collection_find_with_opts (collection, &query, &opts, NULL);

   bson_destroy (&query);
   bson_destroy (&opts);

   return cursor;
}


static void
tail_collection (mongoc_collection_t *collection)
{
   mongoc_cursor_t *cursor;
   uint32_t last_time;
   const bson_t *doc;
   bson_error_t error;
   bson_iter_t iter;

   BSON_ASSERT (collection);

   last_time = (uint32_t) time (NULL);

   while (true) {
      cursor = query_collection (collection, last_time);
      while (!mongoc_cursor_error (cursor, &error) &&
             mongoc_cursor_more (cursor)) {
         if (mongoc_cursor_next (cursor, &doc)) {
            if (bson_iter_init_find (&iter, doc, "ts") &&
                BSON_ITER_HOLDS_TIMESTAMP (&iter)) {
               bson_iter_timestamp (&iter, &last_time, NULL);
            }
            print_bson (doc);
         }
      }
      if (mongoc_cursor_error (cursor, &error)) {
         if (error.domain == MONGOC_ERROR_SERVER) {
            fprintf (stderr, "%s\n", error.message);
            exit (1);
         }
      }

      mongoc_cursor_destroy (cursor);
      sleep (1);
   }
}


int
main (int argc, char *argv[])
{
   mongoc_collection_t *collection;
   mongoc_client_t *client;

   if (argc != 2) {
      fprintf (stderr, "usage: %s MONGO_URI\n", argv[0]);
      return EXIT_FAILURE;
   }

   mongoc_init ();

   client = mongoc_client_new (argv[1]);
   if (!client) {
      fprintf (stderr, "Invalid URI: \"%s\"\n", argv[1]);
      return EXIT_FAILURE;
   }

   mongoc_client_set_error_api (client, 2);

   collection = mongoc_client_get_collection (client, "local", "oplog.rs");

   tail_collection (collection);

   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);

   return 0;
}
