bool
explain (mongoc_collection_t *collection)
{
   bson_t *command;
   bson_t reply;
   bson_error_t error;
   bool res;

   command = BCON_NEW ("explain",
                       "{",
                       "find",
                       BCON_UTF8 (COLLECTION_NAME),
                       "filter",
                       "{",
                       "x",
                       BCON_INT32 (1),
                       "}",
                       "}");
   res = mongoc_collection_command_simple (
      collection, command, NULL, &reply, &error);
   if (!res) {
      fprintf (stderr, "Error with explain: %s\n", error.message);
      goto cleanup;
   }

   /* Do something with the reply */
   print_res (&reply);

cleanup:
   bson_destroy (&reply);
   bson_destroy (command);
   return res;
}
