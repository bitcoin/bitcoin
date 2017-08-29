
void
fam_fields (mongoc_collection_t *collection)
{
   mongoc_find_and_modify_opts_t *opts;
   bson_t fields = BSON_INITIALIZER;
   bson_t *update;
   bson_t reply;
   bson_error_t error;
   bson_t query = BSON_INITIALIZER;
   bool success;


   /* Find Zlatan Ibrahimovic */
   BSON_APPEND_UTF8 (&query, "lastname", "Ibrahimovic");
   BSON_APPEND_UTF8 (&query, "firstname", "Zlatan");

   /* Return his goal tally */
   BSON_APPEND_INT32 (&fields, "goals", 1);

   /* Bump his goal tally */
   update = BCON_NEW ("$inc", "{", "goals", BCON_INT32 (1), "}");

   opts = mongoc_find_and_modify_opts_new ();
   mongoc_find_and_modify_opts_set_update (opts, update);
   mongoc_find_and_modify_opts_set_fields (opts, &fields);
   /* Return the new tally */
   mongoc_find_and_modify_opts_set_flags (opts,
                                          MONGOC_FIND_AND_MODIFY_RETURN_NEW);

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
   bson_destroy (&fields);
   bson_destroy (&query);
   mongoc_find_and_modify_opts_destroy (opts);
}
