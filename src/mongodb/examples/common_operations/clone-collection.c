bool
clone_collection (mongoc_database_t *database, const char *other_host_and_port)
{
   bson_t *command;
   bson_t reply;
   bson_error_t error;
   bool res;

   BSON_ASSERT (other_host_and_port);
   command = BCON_NEW ("cloneCollection",
                       BCON_UTF8 ("test.remoteThings"),
                       "from",
                       BCON_UTF8 (other_host_and_port),
                       "query",
                       "{",
                       "x",
                       BCON_INT32 (1),
                       "}");
   res =
      mongoc_database_command_simple (database, command, NULL, &reply, &error);
   if (!res) {
      fprintf (stderr, "Error with clone: %s\n", error.message);
      goto cleanup;
   }

   /* Do something with the reply */
   print_res (&reply);

cleanup:
   bson_destroy (&reply);
   bson_destroy (command);

   return res;
}
