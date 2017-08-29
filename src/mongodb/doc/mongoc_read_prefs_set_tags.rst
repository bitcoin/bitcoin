:man_page: mongoc_read_prefs_set_tags

mongoc_read_prefs_set_tags()
============================

Synopsis
--------

.. code-block:: c

  void
  mongoc_read_prefs_set_tags (mongoc_read_prefs_t *read_prefs,
                              const bson_t *tags);

Parameters
----------

* ``read_prefs``: A :symbol:`mongoc_read_prefs_t`.
* ``tags``: A :symbol:`bson:bson_t`.

Description
-----------

Sets the tags to be used for the read preference. Only mongod instances matching these tags will be suitable for handling the request.

Examples
--------

.. code-block:: c

   #include <mongoc.h>

   static void
   run_query_with_read_prefs_tags (mongoc_collection_t *collection)
   {
      char *str;
      const bson_t *doc;
      bson_t filter = BSON_INITIALIZER;
      bson_error_t error;
      mongoc_cursor_t *cursor;
      mongoc_read_prefs_t *read_prefs;
      /*  Create a tagset representing
       *  [
       *    {"dc": "ny", "rack": "1" }, // Any node in rack1 in the ny datacenter
       *    {"dc": "ny", "rack": "2" }, // Any node in rack2 in the ny datacenter
       *    {"dc": "ny" },              // Any node in the ny datacenter
       *    {}                          // If all else fails, just any available node
       * ]
       */
      bson_t *tags = BCON_NEW (
         "0", "{", "dc", BCON_UTF8("ny"), "rack", BCON_UTF8("1"), "}",
         "1", "{", "dc", BCON_UTF8("ny"), "rack", BCON_UTF8("2"), "}",
         "2", "{", "dc", BCON_UTF8("ny"), "}",
         "3", "{", "}"
      );

      read_prefs = mongoc_read_prefs_new (MONGOC_READ_SECONDARY);
      mongoc_read_prefs_set_tags (read_prefs, tags);
      bson_destroy (tags);

      cursor =
         mongoc_collection_find_with_opts (collection, &filter, NULL, read_prefs);

      while (mongoc_cursor_next (cursor, &doc)) {
         str = bson_as_canonical_extended_json (doc, NULL);
         printf ("%s\n", str);
         bson_free (str);
      }
      if (mongoc_cursor_error (cursor, &error)) {
         fprintf (stderr, "Cursor error: %s\n", error.message);
      }

      mongoc_cursor_destroy (cursor);
      mongoc_read_prefs_destroy (read_prefs);
      bson_destroy (doc);
   }


   int main (void)
   {
      mongoc_client_t *client;
      mongoc_collection_t *collection;

      mongoc_init ();

      client =
         mongoc_client_new ("mongodb://localhost/?appname=rp_tags&replicaSet=foo");
      mongoc_client_set_error_api (client, 2);
      collection = mongoc_client_get_collection (client, "dbname", "collname");
      run_query_with_read_prefs_tags (collection);

      mongoc_collection_destroy (collection);
      mongoc_client_destroy (client);
      mongoc_cleanup();
   }
