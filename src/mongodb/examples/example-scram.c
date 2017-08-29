/* gcc example.c -o example $(pkg-config --cflags --libs libmongoc-1.0) */

/* ./example-scram */

#include <mongoc.h>
#include <stdio.h>
#include <stdlib.h>

int
main (int argc, char *argv[])
{
   mongoc_client_t *client = NULL;
   mongoc_database_t *database = NULL;
   mongoc_collection_t *collection = NULL;
   mongoc_cursor_t *cursor = NULL;
   bson_error_t error;
   const char *uristr = "mongodb://127.0.0.1/";
   const char *authuristr;
   bson_t roles;
   bson_t query;
   const bson_t *doc;

   if (argc != 2) {
      printf ("%s - [implicit|scram|cr]\n", argv[0]);
      return 1;
   }

   if (strcmp (argv[1], "implicit") == 0) {
      authuristr = "mongodb://user,=:pass@127.0.0.1/test?appname=scram-example";
   } else if (strcmp (argv[1], "scram") == 0) {
      authuristr = "mongodb://user,=:pass@127.0.0.1/"
                   "test?appname=scram-example&authMechanism=SCRAM-SHA-1";
   } else if (strcmp (argv[1], "cr") == 0) {
      authuristr = "mongodb://user,=:pass@127.0.0.1/"
                   "test?appname=scram-example&authMechanism=MONGODB-CR";
   } else {
      printf ("%s - [implicit|scram|cr]\n", argv[0]);
      return 1;
   }

   mongoc_init ();

   client = mongoc_client_new (uristr);

   if (!client) {
      fprintf (stderr, "Failed to parse URI.\n");
      return EXIT_FAILURE;
   }

   mongoc_client_set_error_api (client, 2);

   database = mongoc_client_get_database (client, "test");

   bson_init (&roles);
   bson_init (&query);

   BCON_APPEND (&roles, "0", "{", "role", "root", "db", "admin", "}");

   mongoc_database_add_user (database, "user,=", "pass", &roles, NULL, &error);

   mongoc_database_destroy (database);

   mongoc_client_destroy (client);

   client = mongoc_client_new (authuristr);

   if (!client) {
      fprintf (stderr, "failed to parse SCRAM uri\n");
      goto CLEANUP;
   }

   mongoc_client_set_error_api (client, 2);

   collection = mongoc_client_get_collection (client, "test", "test");

   cursor = mongoc_collection_find_with_opts (collection, &query, NULL, NULL);

   mongoc_cursor_next (cursor, &doc);

   if (mongoc_cursor_error (cursor, &error)) {
      fprintf (stderr, "Auth error: %s\n", error.message);
      goto CLEANUP;
   }

CLEANUP:

   bson_destroy (&roles);
   bson_destroy (&query);

   if (collection) {
      mongoc_collection_destroy (collection);
   }

   if (client) {
      mongoc_client_destroy (client);
   }

   if (cursor) {
      mongoc_cursor_destroy (cursor);
   }

   mongoc_cleanup ();

   return EXIT_SUCCESS;
}
