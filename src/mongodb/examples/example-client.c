/* gcc example-client.c -o example-client $(pkg-config --cflags --libs
 * libmongoc-1.0) */

/* ./example-client [CONNECTION_STRING [COLLECTION_NAME]] */

#include <mongoc.h>
#include <stdio.h>
#include <stdlib.h>

int
main (int argc, char *argv[])
{
   mongoc_client_t *client;
   mongoc_collection_t *collection;
   mongoc_cursor_t *cursor;
   bson_error_t error;
   const bson_t *doc;
   const char *uristr = "mongodb://127.0.0.1/?appname=client-example";
   const char *collection_name = "test";
   bson_t query;
   char *str;

   mongoc_init ();

   if (argc > 1) {
      uristr = argv[1];
   }

   if (argc > 2) {
      collection_name = argv[2];
   }

   client = mongoc_client_new (uristr);

   if (!client) {
      fprintf (stderr, "Failed to parse URI.\n");
      return EXIT_FAILURE;
   }

   mongoc_client_set_error_api (client, 2);

   bson_init (&query);

#if 0
   bson_append_utf8 (&query, "hello", -1, "world", -1);
#endif

   collection = mongoc_client_get_collection (client, "test", collection_name);
   cursor = mongoc_collection_find_with_opts (
      collection,
      &query,
      NULL,  /* additional options */
      NULL); /* read prefs, NULL for default */

   while (mongoc_cursor_next (cursor, &doc)) {
      str = bson_as_canonical_extended_json (doc, NULL);
      fprintf (stdout, "%s\n", str);
      bson_free (str);
   }

   if (mongoc_cursor_error (cursor, &error)) {
      fprintf (stderr, "Cursor Failure: %s\n", error.message);
      return EXIT_FAILURE;
   }

   bson_destroy (&query);
   mongoc_cursor_destroy (cursor);
   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);

   mongoc_cleanup ();

   return EXIT_SUCCESS;
}
