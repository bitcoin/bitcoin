
void
fam_update (mongoc_collection_t *collection)
{
   mongoc_find_and_modify_opts_t *opts;
   bson_t *update;
   bson_t reply;
   bson_error_t error;
   bson_t query = BSON_INITIALIZER;
   bool success;


   /* Find Zlatan Ibrahimovic */
   BSON_APPEND_UTF8 (&query, "firstname", "Zlatan");
   BSON_APPEND_UTF8 (&query, "lastname", "Ibrahimovic");

   /* Make him a book author */
   update = BCON_NEW ("$set", "{", "author", BCON_BOOL (true), "}");

   opts = mongoc_find_and_modify_opts_new ();
   /* Note that the document returned is the _previous_ version of the document
    * To fetch the modified new version, use
    * mongoc_find_and_modify_opts_set_flags (opts,
    * MONGOC_FIND_AND_MODIFY_RETURN_NEW);
    */
   mongoc_find_and_modify_opts_set_update (opts, update);

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
