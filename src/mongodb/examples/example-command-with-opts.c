/*

Demonstrates how to prepare options for mongoc_client_read_command_with_opts and
mongoc_client_write_command_with_opts. First it calls "cloneCollectionAsCapped"
command with "writeConcern" option, then "distinct" command with "collation" and
"readConcern" options,

Start a MongoDB 3.4 replica set with --enableMajorityReadConcern and insert two
documents:

$ mongo
MongoDB Enterprise replset:PRIMARY> db.my_collection.insert({x: 1, y: "One"})
WriteResult({ "nInserted" : 1 })
MongoDB Enterprise replset:PRIMARY> db.my_collection.insert({x: 2, y: "Two"})
WriteResult({ "nInserted" : 1 })

Build and run the example:

gcc example-command-with-opts.c -o example-command-with-opts $(pkg-config
--cflags --libs libmongoc-1.0)
./example-command-with-opts [CONNECTION_STRING]
cloneCollectionAsCapped: { "ok" : 1 }
distinct: { "values" : [ 1, 2 ], "ok" : 1 }

*/

#include <mongoc.h>
#include <stdio.h>
#include <stdlib.h>

int
main (int argc, char *argv[])
{
   mongoc_client_t *client;
   const char *uristr = "mongodb://127.0.0.1/?appname=client-example";
   bson_t *cmd;
   bson_t *opts;
   mongoc_write_concern_t *write_concern;
   mongoc_read_prefs_t *read_prefs;
   mongoc_read_concern_t *read_concern;
   bson_t reply;
   bson_error_t error;
   char *json;

   mongoc_init ();

   if (argc > 1) {
      uristr = argv[1];
   }

   client = mongoc_client_new (uristr);

   if (!client) {
      fprintf (stderr, "Failed to parse URI.\n");
      return EXIT_FAILURE;
   }

   mongoc_client_set_error_api (client, 2);

   cmd = BCON_NEW ("cloneCollectionAsCapped",
                   BCON_UTF8 ("my_collection"),
                   "toCollection",
                   BCON_UTF8 ("my_capped_collection"),
                   "size",
                   BCON_INT64 (1024 * 1024));

   /* include write concern "majority" in command options */
   write_concern = mongoc_write_concern_new ();
   mongoc_write_concern_set_wmajority (write_concern, 10000 /* wtimeoutMS */);
   opts = bson_new ();
   mongoc_write_concern_append (write_concern, opts);

   if (mongoc_client_write_command_with_opts (
          client, "test", cmd, opts, &reply, &error)) {
      json = bson_as_canonical_extended_json (&reply, NULL);
      printf ("cloneCollectionAsCapped: %s\n", json);
      bson_free (json);
   } else {
      fprintf (stderr, "cloneCollectionAsCapped: %s\n", error.message);
   }

   bson_free (cmd);
   bson_free (opts);

   /* distinct values of "x" in "my_collection" where "y" sorts after "one" */
   cmd = BCON_NEW ("distinct",
                   BCON_UTF8 ("my_collection"),
                   "key",
                   BCON_UTF8 ("x"),
                   "query",
                   "{",
                   "y",
                   "{",
                   "$gt",
                   BCON_UTF8 ("one"),
                   "}",
                   "}");

   read_prefs = mongoc_read_prefs_new (MONGOC_READ_SECONDARY);

   /* "One" normally sorts before "one"; make "One" sort after "one" */
   opts = BCON_NEW ("collation",
                    "{",
                    "locale",
                    BCON_UTF8 ("en_US"),
                    "caseFirst",
                    BCON_UTF8 ("lower"),
                    "}");

   /* add a read concern to "opts" */
   read_concern = mongoc_read_concern_new ();
   mongoc_read_concern_set_level (read_concern,
                                  MONGOC_READ_CONCERN_LEVEL_MAJORITY);

   mongoc_read_concern_append (read_concern, opts);

   if (mongoc_client_read_command_with_opts (
          client, "test", cmd, read_prefs, opts, &reply, &error)) {
      json = bson_as_canonical_extended_json (&reply, NULL);
      printf ("distinct: %s\n", json);
      bson_free (json);
   } else {
      fprintf (stderr, "distinct: %s\n", error.message);
   }

   bson_destroy (cmd);
   bson_destroy (opts);
   bson_destroy (&reply);
   mongoc_read_prefs_destroy (read_prefs);
   mongoc_read_concern_destroy (read_concern);
   mongoc_write_concern_destroy (write_concern);
   mongoc_client_destroy (client);

   mongoc_cleanup ();

   return EXIT_SUCCESS;
}
