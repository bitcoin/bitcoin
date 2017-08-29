
void
fam_bypass (mongoc_collection_t *collection)
{
   mongoc_find_and_modify_opts_t *opts;
   bson_t reply;
   bson_t *update;
   bson_error_t error;
   bson_t query = BSON_INITIALIZER;
   bool success;


   /* Find Zlatan Ibrahimovic, the striker */
   BSON_APPEND_UTF8 (&query, "firstname", "Zlatan");
   BSON_APPEND_UTF8 (&query, "lastname", "Ibrahimovic");
   BSON_APPEND_UTF8 (&query, "profession", "Football player");

   /* Bump his age */
   update = BCON_NEW ("$inc", "{", "age", BCON_INT32 (1), "}");

   opts = mongoc_find_and_modify_opts_new ();
   mongoc_find_and_modify_opts_set_update (opts, update);
   /* He can still play, even though he is pretty old. */
   mongoc_find_and_modify_opts_set_bypass_document_validation (opts, true);

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
