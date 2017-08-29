bool
map_reduce_basic (mongoc_database_t *database)
{
   bson_t reply;
   bson_t *command;
   bool res;
   bson_error_t error;
   mongoc_cursor_t *cursor;
   const bson_t *doc;

   bool map_reduce_done = false;
   bool query_done = false;

   const char *out_collection_name = "outCollection";
   mongoc_collection_t *out_collection;

   /* Empty find query */
   bson_t find_query = BSON_INITIALIZER;

   /* Construct the mapReduce command */

   /* Other arguments can also be specified here, like "query" or
      "limit" and so on */
   command = BCON_NEW ("mapReduce",
                       BCON_UTF8 (COLLECTION_NAME),
                       "map",
                       BCON_CODE (MAPPER),
                       "reduce",
                       BCON_CODE (REDUCER),
                       "out",
                       BCON_UTF8 (out_collection_name));
   res =
      mongoc_database_command_simple (database, command, NULL, &reply, &error);
   map_reduce_done = true;

   if (!res) {
      fprintf (stderr, "MapReduce failed: %s\n", error.message);
      goto cleanup;
   }

   /* Do something with the reply (it doesn't contain the mapReduce results) */
   print_res (&reply);

   /* Now we'll query outCollection to see what the results are */
   out_collection =
      mongoc_database_get_collection (database, out_collection_name);
   cursor = mongoc_collection_find_with_opts (
      out_collection, &find_query, NULL, NULL);
   query_done = true;

   /* Do something with the results */
   while (mongoc_cursor_next (cursor, &doc)) {
      print_res (doc);
   }

   if (mongoc_cursor_error (cursor, &error)) {
      fprintf (stderr, "ERROR: %s\n", error.message);
      res = false;
      goto cleanup;
   }

cleanup:
   /* cleanup */
   if (query_done) {
      mongoc_cursor_destroy (cursor);
      mongoc_collection_destroy (out_collection);
   }

   if (map_reduce_done) {
      bson_destroy (&reply);
      bson_destroy (command);
   }

   return res;
}
