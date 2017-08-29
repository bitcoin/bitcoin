
#include <bcon.h>
#include <mongoc.h>

#include "flags.c"
#include "bypass.c"
#include "update.c"
#include "fields.c"
#include "opts.c"
#include "sort.c"

int
main (void)
{
   mongoc_collection_t *collection;
   mongoc_database_t *database;
   mongoc_client_t *client;
   bson_error_t error;
   bson_t *options;

   mongoc_init ();
   client = mongoc_client_new (
      "mongodb://localhost:27017/admin?appname=find-and-modify-opts-example");
   mongoc_client_set_error_api (client, 2);
   database = mongoc_client_get_database (client, "databaseName");

   options = BCON_NEW ("validator",
                       "{",
                       "age",
                       "{",
                       "$lte",
                       BCON_INT32 (34),
                       "}",
                       "}",
                       "validationAction",
                       BCON_UTF8 ("error"),
                       "validationLevel",
                       BCON_UTF8 ("moderate"));

   collection = mongoc_database_create_collection (
      database, "collectionName", options, &error);
   if (!collection) {
      fprintf (
         stderr, "Got error: \"%s\" on line %d\n", error.message, __LINE__);
      return 1;
   }

   fam_flags (collection);
   fam_bypass (collection);
   fam_update (collection);
   fam_fields (collection);
   fam_opts (collection);
   fam_sort (collection);

   mongoc_collection_drop (collection, NULL);
   bson_destroy (options);
   mongoc_database_destroy (database);
   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);

   mongoc_cleanup ();
   return 0;
}
