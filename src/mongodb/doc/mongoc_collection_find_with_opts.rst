:man_page: mongoc_collection_find_with_opts

mongoc_collection_find_with_opts()
==================================

Synopsis
--------

.. code-block:: c

  mongoc_cursor_t *
  mongoc_collection_find_with_opts (mongoc_collection_t *collection,
                                    const bson_t *filter,
                                    const bson_t *opts,
                                    const mongoc_read_prefs_t *read_prefs)
     BSON_GNUC_WARN_UNUSED_RESULT;

Parameters
----------

* ``collection``: A :symbol:`mongoc_collection_t`.
* ``filter``: A :symbol:`bson:bson_t` containing the query to execute.
* ``opts``: A :symbol:`bson:bson_t` query options, including sort order and which fields to return. Can be ``NULL``.
* ``read_prefs``: A :symbol:`mongoc_read_prefs_t` or ``NULL``.

Description
-----------

Query on ``collection``, passing arbitrary query options to the server in ``opts``.

To target a specific server, include an integer "serverId" field in ``opts`` with an id obtained first by calling :symbol:`mongoc_client_select_server`, then :symbol:`mongoc_server_description_id` on its return value.

Returns
-------

A newly allocated :symbol:`mongoc_cursor_t` that must be freed with :symbol:`mongoc_cursor_destroy()`.

Examples
--------

.. code-block:: c
  :caption: Print First Ten Documents in a Collection

  #include <mongoc.h>
  #include <stdio.h>

  static void
  print_ten_documents (mongoc_collection_t *collection)
  {
     bson_t *filter;
     bson_t *opts;
     mongoc_cursor_t *cursor;
     bson_error_t error;
     const bson_t *doc;
     char *str;

     /* filter by "foo": 1, order by "bar" descending */
     filter = BCON_NEW ("foo", BCON_INT32 (1));
     opts = BCON_NEW (
        "limit", BCON_INT64 (10), "sort", "{", "bar", BCON_INT32 (-1), "}");

     cursor = mongoc_collection_find_with_opts (collection, filter, opts, NULL);

     while (mongoc_cursor_next (cursor, &doc)) {
        str = bson_as_canonical_extended_json (doc, NULL);
        printf ("%s\n", str);
        bson_free (str);
     }

     if (mongoc_cursor_error (cursor, &error)) {
        fprintf (stderr, "An error occurred: %s\n", error.message);
     }

     mongoc_cursor_destroy (cursor);
     bson_destroy (filter);
     bson_destroy (opts);
  }

.. code-block:: c
  :caption: More examples of modifying the query with ``opts``:

  bson_t *filter;
  bson_t *opts;
  mongoc_read_prefs_t *read_prefs;

  filter = BCON_NEW ("foo", BCON_INT32 (1));

  /* Include "field_name_one" and "field_name_two" in "projection", omit
   * others. "_id" must be specifically removed or it is included by default.
   */
  opts = BCON_NEW ("projection", "{",
                      "field_name_one", BCON_BOOL (true),
                      "field_name_two", BCON_BOOL (true),
                      "_id", BCON_BOOL (false),
                   "}",
                   "tailable", BCON_BOOL (true),
                   "awaitData", BCON_BOOL (true),
                   "sort", "{", "bar", BCON_INT32 (-1), "}",
                   "collation", "{",
                      "locale", BCON_UTF8("en_US"),
                      "caseFirst", BCON_UTF8 ("lower"),
                   "}");

  read_prefs = mongoc_read_prefs_new (MONGOC_READ_SECONDARY);

  cursor =
     mongoc_collection_find_with_opts (collection, filter, opts, read_prefs);

The following options are supported.

=======================  ==================  ===================  ==================
Option                   BSON type           Option               BSON type
=======================  ==================  ===================  ==================
``projection``           document            ``maxScan``          non-negative int64
``sort``                 document            ``maxTimeMS``        non-negative int64
``skip``                 non-negative int64  ``maxAwaitTimeMS``   non-negative int64
``limit``                non-negative int64  ``min``              document
``batchSize``            non-negative int64  ``noCursorTimeout``  bool
``exhaust``              bool                ``oplogReplay``      bool
``hint``                 string or document  ``returnKey``        bool
``allowPartialResults``  bool                ``showRecordId``     bool
``awaitData``            bool                ``singleBatch``      bool
``collation``            document            ``snapshot``         bool
``comment``              string              ``tailable``         bool              
``max``                  document
=======================  ==================  ===================  ==================

All options are documented in the reference page for `the "find" command`_ in the MongoDB server manual, except for "maxAwaitTimeMS".

"maxAwaitTimeMS" is the maximum amount of time for the server to wait on new documents to satisfy a query, if "tailable" and "awaitData" are both true.
If no new documents are found, the tailable cursor receives an empty batch. The "maxAwaitTimeMS" option is ignored for MongoDB older than 3.4.

For some options like "collation", the driver returns an error if the server version is too old to support the feature.
Any fields in ``opts`` that are not listed here are passed to the server unmodified.

See Also
--------

`The "find" command`_ in the MongoDB Manual. All options listed there are supported by the C Driver.
For MongoDB servers before 3.2, or for exhaust queries, the driver transparently converts the query to a legacy OP_QUERY message.

.. _the "find" command: https://docs.mongodb.org/master/reference/command/find/

The "explain" command
---------------------

With MongoDB before 3.2, a query with option ``$explain: true`` returns information about the query plan, instead of the query results. Beginning in MongoDB 3.2, there is a separate "explain" command. The driver will not convert "$explain" queries to "explain" commands, you must call the "explain" command explicitly:

.. code-block:: c

  /* MongoDB 3.2+, "explain" command syntax */
  command = BCON_NEW ("explain", "{",
                      "find", BCON_UTF8 ("collection_name"),
                      "filter", "{", "foo", BCON_INT32 (1), "}",
                      "}");

  mongoc_collection_command_simple (collection, command, NULL, &reply, &error);

See Also
--------

`The "explain" command <https://docs.mongodb.org/master/reference/command/explain/>`_ in the MongoDB Manual.

