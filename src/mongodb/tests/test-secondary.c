#include <mongoc.h>

static bool gExpectingFailure;
static bool gShutdown;

static void
query_collection (mongoc_collection_t *col)
{
   mongoc_cursor_t *cursor;
   const bson_t *doc;
   bson_error_t error;
   bson_t q;

   mongoc_init ();

   bson_init (&q);
   bson_append_utf8 (&q, "hello", -1, "world", -1);

   cursor =
      mongoc_collection_find (col, MONGOC_QUERY_NONE, 0, 0, 0, &q, NULL, NULL);

   while (mongoc_cursor_next (cursor, &doc)) {
      char *str;

      str = bson_as_canonical_extended_json (doc, NULL);
      fprintf (stderr, "%s\n", str);
      bson_free (str);
   }

   if (mongoc_cursor_error (cursor, &error)) {
      if (gExpectingFailure) {
         if ((error.domain != MONGOC_ERROR_STREAM) ||
             (error.code != MONGOC_ERROR_STREAM_SOCKET)) {
            abort ();
         }
         gExpectingFailure = false;
      } else {
         fprintf (stderr, "%s", error.message);
         abort ();
      }
   }

   bson_destroy (&q);
}

static void
test_secondary (mongoc_client_t *client)
{
   mongoc_collection_t *col;

   col = mongoc_client_get_collection (client, "test", "test");

   while (!gShutdown) {
      query_collection (col);
   }

   mongoc_collection_destroy (col);
}

int
main (int argc, char *argv[])
{
   mongoc_read_prefs_t *read_prefs;
   mongoc_client_t *client;
   mongoc_uri_t *uri;

   if (argc < 2) {
      fprintf (stderr, "usage: %s mongodb://...\n", argv[0]);
      return EXIT_FAILURE;
   }

   uri = mongoc_uri_new (argv[1]);
   if (!uri) {
      fprintf (stderr, "Invalid URI: \"%s\"\n", argv[1]);
      return EXIT_FAILURE;
   }

   client = mongoc_client_new_from_uri (uri);

   read_prefs = mongoc_read_prefs_new (MONGOC_READ_SECONDARY);
   mongoc_client_set_read_prefs (client, read_prefs);
   mongoc_read_prefs_destroy (read_prefs);

   test_secondary (client);

   mongoc_client_destroy (client);
   mongoc_uri_destroy (uri);

   return EXIT_SUCCESS;
}
