/*
 * Copyright 2014 MongoDB, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#include <bson.h>
#include <fcntl.h>
#include <mongoc.h>


static bool
mongoc_dump_mkdir_p (const char *path, int mode)
{
   int r;

#ifdef _WIN32
   r = _mkdir (path);
#else
   r = mkdir (path, mode);
#endif

   return (r == 0 || errno == EEXIST);
}


static int
mongoc_dump_collection (mongoc_client_t *client,
                        const char *database,
                        const char *collection)
{
   mongoc_collection_t *col;
   mongoc_cursor_t *cursor;
   const bson_t *doc;
   bson_error_t error;
   bson_t query = BSON_INITIALIZER;
   FILE *stream;
   char *path;
   int ret = EXIT_SUCCESS;

   path = bson_strdup_printf ("dump/%s/%s.bson", database, collection);
#ifdef _WIN32
   _unlink (path);
#else
   unlink (path);
#endif

   stream = fopen (path, "w");
   if (!stream) {
      fprintf (stderr, "Failed to open \"%s\", aborting.\n", path);
      exit (EXIT_FAILURE);
   }

   col = mongoc_client_get_collection (client, database, collection);
   cursor = mongoc_collection_find_with_opts (col, &query, NULL, NULL);

   while (mongoc_cursor_next (cursor, &doc)) {
      if (BSON_UNLIKELY (doc->len !=
                         fwrite (bson_get_data (doc), 1, doc->len, stream))) {
         fprintf (stderr, "Failed to write %u bytes to %s\n", doc->len, path);
         ret = EXIT_FAILURE;
         goto cleanup;
      }
   }

   if (mongoc_cursor_error (cursor, &error)) {
      fprintf (stderr, "ERROR: %s\n", error.message);
      ret = EXIT_FAILURE;
   }

cleanup:
   bson_free (path);
   fclose (stream);
   mongoc_cursor_destroy (cursor);
   mongoc_collection_destroy (col);

   return ret;
}


static int
mongoc_dump_database (mongoc_client_t *client,
                      const char *database,
                      const char *collection)
{
   mongoc_database_t *db;
   bson_error_t error;
   char *path;
   char **str;
   int ret = EXIT_SUCCESS;
   int i;

   BSON_ASSERT (database);

   path = bson_strdup_printf ("dump/%s", database);
   if (!mongoc_dump_mkdir_p (path, 0750)) {
      fprintf (stderr, "failed to create directory \"%s\"", path);
      bson_free (path);
      return EXIT_FAILURE;
   }

   bson_free (path);

   if (collection) {
      return mongoc_dump_collection (client, database, collection);
   }

   db = mongoc_client_get_database (client, database);
   str = mongoc_database_get_collection_names (db, &error);
   for (i = 0; str[i]; i++) {
      if (EXIT_SUCCESS != mongoc_dump_collection (client, database, str[i])) {
         ret = EXIT_FAILURE;
         goto cleanup;
      }
   }

cleanup:
   mongoc_database_destroy (db);
   bson_strfreev (str);

   return ret;
}


static int
mongoc_dump (mongoc_client_t *client,
             const char *database,
             const char *collection)
{
   bson_error_t error;
   char **str;
   int i;

   if (!mongoc_dump_mkdir_p ("dump", 0750)) {
      perror ("Failed to create directory \"dump\"");
      return EXIT_FAILURE;
   }

   if (database) {
      return mongoc_dump_database (client, database, collection);
   }

   if (!(str = mongoc_client_get_database_names (client, &error))) {
      fprintf (stderr, "Failed to fetch database names: %s\n", error.message);
      return EXIT_FAILURE;
   }

   for (i = 0; str[i]; i++) {
      if (EXIT_SUCCESS != mongoc_dump_database (client, str[i], NULL)) {
         bson_strfreev (str);
         return EXIT_FAILURE;
      }
   }

   bson_strfreev (str);

   return EXIT_SUCCESS;
}


static void
usage (FILE *stream)
{
   fprintf (stream,
            "Usage: mongoc-dump [OPTIONS]\n"
            "\n"
            "Options:\n"
            "\n"
            "  -h HOST      Optional hostname to connect to [127.0.0.1].\n"
            "  -p PORT      Optional port to connect to [27017].\n"
            "  -d DBNAME    Optional database name to dump.\n"
            "  -c COLNAME   Optional collection name to dump.\n"
            "  --ssl        Use SSL when connecting to server.\n"
            "\n");
}


int
main (int argc, char *argv[])
{
   mongoc_client_t *client;
   const char *collection = NULL;
   const char *database = NULL;
   const char *host = "127.0.0.1";
   uint16_t port = 27017;
   bool ssl = false;
   char *uri;
   int ret;
   int i;

   mongoc_init ();

   for (i = 1; i < argc; i++) {
      if (0 == strcmp (argv[i], "-c") && ((i + 1) < argc)) {
         collection = argv[++i];
      } else if (0 == strcmp (argv[i], "-d") && ((i + 1) < argc)) {
         database = argv[++i];
      } else if (0 == strcmp (argv[i], "--help")) {
         usage (stdout);
         return EXIT_SUCCESS;
      } else if (0 == strcmp (argv[i], "-h") && ((i + 1) < argc)) {
         host = argv[++i];
      } else if (0 == strcmp (argv[i], "--ssl")) {
         ssl = true;
      } else if (0 == strcmp (argv[i], "-p") && ((i + 1) < argc)) {
         port = atoi (argv[++i]);
         if (!port) {
            fprintf (stderr, "Invalid port \"%s\"", argv[i]);
            return EXIT_FAILURE;
         }
      } else {
         fprintf (stderr, "Unknown argument \"%s\"\n", argv[i]);
         return EXIT_FAILURE;
      }
   }

   uri = bson_strdup_printf ("mongodb://%s:%hu/%s?appname=dump-example&ssl=%s",
                             host,
                             port,
                             database ? database : "",
                             ssl ? "true" : "false");

   if (!(client = mongoc_client_new (uri))) {
      fprintf (stderr, "Invalid connection URI: %s\n", uri);
      return EXIT_FAILURE;
   }

   mongoc_client_set_error_api (client, 2);

   ret = mongoc_dump (client, database, collection);

   mongoc_client_destroy (client);

   return ret;
}
