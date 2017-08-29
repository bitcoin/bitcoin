bool
distinct (mongoc_database_t *database)
{
   bson_t *command;
   bson_t reply;
   bson_error_t error;
   bool res;
   bson_iter_t iter;
   bson_iter_t array_iter;
   double val;

   command = BCON_NEW ("distinct",
                       BCON_UTF8 (COLLECTION_NAME),
                       "key",
                       BCON_UTF8 ("x"),
                       "query",
                       "{",
                       "x",
                       "{",
                       "$gt",
                       BCON_DOUBLE (1.0),
                       "}",
                       "}");
   res =
      mongoc_database_command_simple (database, command, NULL, &reply, &error);
   if (!res) {
      fprintf (stderr, "Error with distinct: %s\n", error.message);
      goto cleanup;
   }

   /* Do something with reply (in this case iterate through the values) */
   if (!(bson_iter_init_find (&iter, &reply, "values") &&
         BSON_ITER_HOLDS_ARRAY (&iter) &&
         bson_iter_recurse (&iter, &array_iter))) {
      fprintf (stderr, "Couldn't extract \"values\" field from response\n");
      goto cleanup;
   }

   while (bson_iter_next (&array_iter)) {
      if (BSON_ITER_HOLDS_DOUBLE (&array_iter)) {
         val = bson_iter_double (&array_iter);
         printf ("Next double: %f\n", val);
      }
   }

cleanup:
   /* cleanup */
   bson_destroy (command);
   bson_destroy (&reply);
   return res;
}
