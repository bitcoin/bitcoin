#include <mongoc.h>
#include <mongoc-client-private.h>


static void
print_doc (const bson_t *b)
{
   char *str;

   str = bson_as_canonical_extended_json (b, NULL);
   MONGOC_DEBUG ("%s", str);
   bson_free (str);
}


static void
ping (mongoc_database_t *db, const bson_t *cmd)
{
   mongoc_cursor_t *cursor;
   const bson_t *b;
   bson_error_t error;

   cursor =
      mongoc_database_command (db, MONGOC_QUERY_NONE, 0, 1, 0, cmd, NULL, NULL);
   while (mongoc_cursor_next (cursor, &b)) {
      BSON_ASSERT (b);
      print_doc (b);
   }
   if (mongoc_cursor_error (cursor, &error)) {
      MONGOC_WARNING ("Cursor error: %s", error.message);
   }
   mongoc_cursor_destroy (cursor);
}


static void
fetch (mongoc_collection_t *col, const bson_t *spec)
{
   mongoc_cursor_t *cursor;
   const bson_t *b;
   bson_error_t error;

   cursor = mongoc_collection_find_with_opts (col, spec, NULL, NULL);
   while (mongoc_cursor_next (cursor, &b)) {
      BSON_ASSERT (b);
      print_doc (b);
   }
   if (mongoc_cursor_error (cursor, &error)) {
      MONGOC_WARNING ("Cursor error: %s", error.message);
   }
   mongoc_cursor_destroy (cursor);
}


static void
test_load (mongoc_client_t *client, unsigned iterations)
{
   mongoc_collection_t *col;
   mongoc_database_t *db;
   bson_error_t error;
   unsigned i;
   bson_t b;
   bson_t q;

   bson_init (&b);
   bson_append_int32 (&b, "ping", 4, 1);

   bson_init (&q);

   db = mongoc_client_get_database (client, "admin");
   col = mongoc_client_get_collection (client, "test", "test");

   for (i = 0; i < iterations; i++) {
      ping (db, &b);
      fetch (col, &q);
   }

   if (!mongoc_collection_drop (col, &error)) {
      MONGOC_WARNING ("Failed to drop collection: %s", error.message);
   }

   mongoc_database_destroy (db);
   db = mongoc_client_get_database (client, "test");

   if (!mongoc_database_drop (db, &error)) {
      MONGOC_WARNING ("Failed to drop database: %s", error.message);
   }

   mongoc_database_destroy (db);
   mongoc_collection_destroy (col);
   bson_destroy (&b);
}


int
main (int argc, char *argv[])
{
   mongoc_client_pool_t *pool;
   mongoc_client_t *client;
   mongoc_uri_t *uri;
   unsigned count = 10000000;

   mongoc_init ();

   if (argc > 1) {
      if (!(uri = mongoc_uri_new (argv[1]))) {
         fprintf (stderr, "Failed to parse uri: %s\n", argv[1]);
         return 1;
      }
   } else {
      uri = mongoc_uri_new ("mongodb://127.0.0.1:27017/?sockettimeoutms=500");
   }

   if (argc > 2) {
      count = BSON_MAX (atoi (argv[2]), 1);
   }

   pool = mongoc_client_pool_new (uri);
   client = mongoc_client_pool_pop (pool);
   test_load (client, count);
   mongoc_client_pool_push (pool, client);
   mongoc_uri_destroy (uri);
   mongoc_client_pool_destroy (pool);

   mongoc_cleanup ();

   return 0;
}
