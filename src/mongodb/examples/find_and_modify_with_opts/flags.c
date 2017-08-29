
void
fam_flags (mongoc_collection_t *collection)
{
   mongoc_find_and_modify_opts_t *opts;
   bson_t reply;
   bson_error_t error;
   bson_t query = BSON_INITIALIZER;
   bson_t *update;
   bool success;


   /* Find Zlatan Ibrahimovic, the striker */
   BSON_APPEND_UTF8 (&query, "firstname", "Zlatan");
   BSON_APPEND_UTF8 (&query, "lastname", "Ibrahimovic");
   BSON_APPEND_UTF8 (&query, "profession", "Football player");
   BSON_APPEND_INT32 (&query, "age", 34);
   BSON_APPEND_INT32 (
      &query, "goals", (16 + 35 + 23 + 57 + 16 + 14 + 28 + 84) + (1 + 6 + 62));

   /* Add his football position */
   update = BCON_NEW ("$set", "{", "position", BCON_UTF8 ("striker"), "}");

   opts = mongoc_find_and_modify_opts_new ();

   mongoc_find_and_modify_opts_set_update (opts, update);

   /* Create the document if it didn't exist, and return the updated document */
   mongoc_find_and_modify_opts_set_flags (
      opts, MONGOC_FIND_AND_MODIFY_UPSERT | MONGOC_FIND_AND_MODIFY_RETURN_NEW);

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
   bson_destroy (update);
   bson_destroy (&query);
   mongoc_find_and_modify_opts_destroy (opts);
}
