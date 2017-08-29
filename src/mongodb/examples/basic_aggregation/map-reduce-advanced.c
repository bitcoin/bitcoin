bool
map_reduce_advanced (mongoc_database_t *database)
{
   bson_t *command;
   bson_error_t error;
   bool res = true;
   mongoc_cursor_t *cursor;
   mongoc_read_prefs_t *read_pref;
   const bson_t *doc;

   /* Construct the mapReduce command */
   /* Other arguments can also be specified here, like "query" or "limit"
      and so on */

   /* Read the results inline from a secondary replica */
   command = BCON_NEW ("mapReduce",
                       BCON_UTF8 (COLLECTION_NAME),
                       "map",
                       BCON_CODE (MAPPER),
                       "reduce",
                       BCON_CODE (REDUCER),
                       "out",
                       "{",
                       "inline",
                       "1",
                       "}");

   read_pref = mongoc_read_prefs_new (MONGOC_READ_SECONDARY);
   cursor = mongoc_database_command (
      database, MONGOC_QUERY_NONE, 0, 0, 0, command, NULL, read_pref);

   /* Do something with the results */
   while (mongoc_cursor_next (cursor, &doc)) {
      print_res (doc);
   }

   if (mongoc_cursor_error (cursor, &error)) {
      fprintf (stderr, "ERROR: %s\n", error.message);
      res = false;
   }

   mongoc_cursor_destroy (cursor);
   mongoc_read_prefs_destroy (read_pref);
   bson_destroy (command);

   return res;
}
