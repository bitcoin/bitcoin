/* gcc example-create-indexes.c -o example-create-indexes $(pkg-config --cflags
 * --libs libmongoc-1.0) */

/* ./example-create-indexes [CONNECTION_STRING [COLLECTION_NAME]] */

#include <mongoc.h>
#include <stdio.h>
#include <stdlib.h>

int
main (int argc, char *argv[])
{
   mongoc_client_t *client;
   mongoc_database_t *db;
   const char *uristr = "mongodb://127.0.0.1/?appname=create-indexes-example";
   const char *collection_name = "test";
   bson_t keys;
   char *index_name;
   bson_t *create_indexes;
   bson_t reply;
   char *reply_str;
   bson_error_t error;
   bool r;

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
   db = mongoc_client_get_database (client, "test");

   /* ascending index on field "x" */
   bson_init (&keys);
   BSON_APPEND_INT32 (&keys, "x", 1);
   index_name = mongoc_collection_keys_to_index_string (&keys);
   create_indexes = BCON_NEW ("createIndexes",
                              BCON_UTF8 (collection_name),
                              "indexes",
                              "[",
                              "{",
                              "key",
                              BCON_DOCUMENT (&keys),
                              "name",
                              BCON_UTF8 (index_name),
                              "}",
                              "]");

   r = mongoc_database_write_command_with_opts (
      db, create_indexes, NULL /* opts */, &reply, &error);

   reply_str = bson_as_json (&reply, NULL);
   printf ("%s\n", reply_str);

   if (!r) {
      fprintf (stderr, "Error in createIndexes: %s\n", error.message);
   }

   bson_free (index_name);
   bson_free (reply_str);
   bson_destroy (&reply);
   bson_destroy (create_indexes);
   mongoc_database_destroy (db);
   mongoc_client_destroy (client);

   mongoc_cleanup ();

   return r ? EXIT_SUCCESS : EXIT_FAILURE;
}
