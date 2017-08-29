bool
copydb (mongoc_client_t *client, const char *other_host_and_port)
{
   mongoc_database_t *admindb;
   bson_t *command;
   bson_t reply;
   bson_error_t error;
   bool res;

   BSON_ASSERT (other_host_and_port);
   /* Must do this from the admin db */
   admindb = mongoc_client_get_database (client, "admin");

   command = BCON_NEW ("copydb",
                       BCON_INT32 (1),
                       "fromdb",
                       BCON_UTF8 ("test"),
                       "todb",
                       BCON_UTF8 ("test2"),

                       /* If you want from a different host */
                       "fromhost",
                       BCON_UTF8 (other_host_and_port));
   res =
      mongoc_database_command_simple (admindb, command, NULL, &reply, &error);
   if (!res) {
      fprintf (stderr, "Error with copydb: %s\n", error.message);
      goto cleanup;
   }

   /* Do something with the reply */
   print_res (&reply);

cleanup:
   bson_destroy (&reply);
   bson_destroy (command);
   mongoc_database_destroy (admindb);

   return res;
}
