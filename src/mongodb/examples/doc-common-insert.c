/* Don't try to compile this file on its own. It's meant to be #included
   by example code */

/* Insert some sample data */
bool
insert_data (mongoc_collection_t *collection)
{
   mongoc_bulk_operation_t *bulk;
   enum N { ndocs = 4 };
   bson_t *docs[ndocs];
   bson_error_t error;
   int i = 0;
   bool ret;

   bulk = mongoc_collection_create_bulk_operation (collection, true, NULL);

   docs[0] = BCON_NEW ("x", BCON_DOUBLE (1.0), "tags", "[", "dog", "cat", "]");
   docs[1] = BCON_NEW ("x", BCON_DOUBLE (2.0), "tags", "[", "cat", "]");
   docs[2] = BCON_NEW (
      "x", BCON_DOUBLE (2.0), "tags", "[", "mouse", "cat", "dog", "]");
   docs[3] = BCON_NEW ("x", BCON_DOUBLE (3.0), "tags", "[", "]");

   for (i = 0; i < ndocs; i++) {
      mongoc_bulk_operation_insert (bulk, docs[i]);
      bson_destroy (docs[i]);
      docs[i] = NULL;
   }

   ret = mongoc_bulk_operation_execute (bulk, NULL, &error);

   if (!ret) {
      fprintf (stderr, "Error inserting data: %s\n", error.message);
   }

   mongoc_bulk_operation_destroy (bulk);
   return ret;
}

/* A helper which we'll use a lot later on */
void
print_res (const bson_t *reply)
{
   BSON_ASSERT (reply);
   char *str = bson_as_canonical_extended_json (reply, NULL);
   printf ("%s\n", str);
   bson_free (str);
}
