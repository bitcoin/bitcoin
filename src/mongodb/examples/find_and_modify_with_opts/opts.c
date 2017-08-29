
void
fam_opts (mongoc_collection_t *collection)
{
   mongoc_find_and_modify_opts_t *opts;
   bson_t reply;
   bson_t *update;
   bson_error_t error;
   bson_t query = BSON_INITIALIZER;
   bson_t extra = BSON_INITIALIZER;
   bool success;


   /* Find Zlatan Ibrahimovic, the striker */
   BSON_APPEND_UTF8 (&query, "firstname", "Zlatan");
   BSON_APPEND_UTF8 (&query, "lastname", "Ibrahimovic");
   BSON_APPEND_UTF8 (&query, "profession", "Football player");

   /* Bump his age */
   update = BCON_NEW ("$inc", "{", "age", BCON_INT32 (1), "}");

   opts = mongoc_find_and_modify_opts_new ();
   mongoc_find_and_modify_opts_set_update (opts, update);

   /* Abort if the operation takes too long. */
   mongoc_find_and_modify_opts_set_max_time_ms (opts, 100);

   /* Some future findAndModify option the driver doesn't support conveniently
    */
   BSON_APPEND_INT32 (&extra, "futureOption", 42);
   mongoc_find_and_modify_opts_append (opts, &extra);

   success = mongoc_collection_find_and_modify_with_opts (
      collection, &query, opts, &reply, &error);

   if (success) {
      char *str;

      str = bson_as_canonical_extended_json (&reply, NULL);
      printf ("%s\n", str);
      bson_free (str);
   } else {
      fprintf (
         stderr, "Got error: \"%s\" on line %d\n", error.message, __LINE__);
   }

   bson_destroy (&reply);
   bson_destroy (&extra);
   bson_destroy (update);
   bson_destroy (&query);
   mongoc_find_and_modify_opts_destroy (opts);
}
