:man_page: mongoc_collection_find

mongoc_collection_find()
========================

Deprecated
----------

This function is deprecated and should not be used in new code.

Use the more convenient :symbol:`mongoc_collection_find_with_opts` instead.

Synopsis
--------

.. code-block:: c

  mongoc_cursor_t *
  mongoc_collection_find (mongoc_collection_t *collection,
                          mongoc_query_flags_t flags,
                          uint32_t skip,
                          uint32_t limit,
                          uint32_t batch_size,
                          const bson_t *query,
                          const bson_t *fields,
                          const mongoc_read_prefs_t *read_prefs)
     BSON_GNUC_DEPRECATED_FOR (mongoc_collection_find_with_opts)
        BSON_GNUC_WARN_UNUSED_RESULT;

Parameters
----------

* ``collection``: A :symbol:`mongoc_collection_t`.
* ``flags``: A :symbol:`mongoc_query_flags_t`.
* ``skip``: A uint32_t of number of documents to skip or 0.
* ``limit``: A uint32_t of max number of documents to return or 0.
* ``batch_size``: A uint32_t containing batch size of document result sets or 0 for default. Default is 100.
* ``query``: A :symbol:`bson:bson_t` containing the query and options to execute.
* ``fields``: A :symbol:`bson:bson_t` containing fields to return or ``NULL``.
* ``read_prefs``: A :symbol:`mongoc_read_prefs_t` or ``NULL`` for default read preferences.

Description
-----------

This function shall execute a query on the underlying ``collection``.

If no options are necessary, ``query`` can simply contain a query such as ``{a:1}``. If you would like to specify options such as a sort order, the query must be placed inside of ``{"$query": {}}``. See the example below for how to properly specify additional options to ``query``.

Returns
-------

A newly allocated :symbol:`mongoc_cursor_t` that should be freed with :symbol:`mongoc_cursor_destroy()` when no longer in use.

Example
-------

.. code-block:: c
  :caption: Print All Documents in a Collection

  #include <mongoc.h>
  #include <stdio.h>

  static void
  print_all_documents (mongoc_collection_t *collection)
  {
     mongoc_cursor_t *cursor;
     bson_error_t error;
     const bson_t *doc;
     char *str;
     bson_t *query;

     query = BCON_NEW ("$query",
                       "{",
                       "foo",
                       BCON_INT32 (1),
                       "}",
                       "$orderby",
                       "{",
                       "bar",
                       BCON_INT32 (-1),
                       "}");
     cursor = mongoc_collection_find (
        collection, MONGOC_QUERY_NONE, 0, 0, 0, query, NULL, NULL);

     while (mongoc_cursor_next (cursor, &doc)) {
        str = bson_as_canonical_extended_json (doc, NULL);
        printf ("%s\n", str);
        bson_free (str);
     }

     if (mongoc_cursor_error (cursor, &error)) {
        fprintf (stderr, "An error occurred: %s\n", error.message);
     }

     mongoc_cursor_destroy (cursor);
     bson_destroy (query);
  }

The "find" command
------------------

Queries have historically been sent as OP_QUERY wire protocol messages, but beginning in MongoDB 3.2 queries use `the "find" command <https://docs.mongodb.org/master/reference/command/find/>`_ instead.

The driver automatically converts queries to the new "find" command syntax if needed, so this change is typically invisible to C Driver users. However, an application written exclusively for MongoDB 3.2 and later can choose to use the new syntax directly instead of relying on the driver to convert from the old syntax:

.. code-block:: c

  /* MongoDB 3.2+ "find" command syntax */
  query = BCON_NEW ("filter",
                    "{",
                    "foo",
                    BCON_INT32 (1),
                    "}",
                    "sort",
                    "{",
                    "bar",
                    BCON_INT32 (-1),
                    "}");
  cursor = mongoc_collection_find (
     collection, MONGOC_QUERY_NONE, 0, 0, 0, query, NULL, NULL);

The "find" command takes different options from the traditional OP_QUERY message.

====================  ==================  =================
Query                 ``$query``          ``filter``       
Sort                  ``$orderby``        ``sort``         
Show record location  ``$showDiskLoc``    ``showRecordId`` 
Other $-options       ``$<option name>``  ``<option name>``
====================  ==================  =================

Most applications should use the OP_QUERY syntax, with "$query", "$orderby", and so on, and rely on the driver to convert to the new syntax if needed.

See Also
--------

`The "find" command <https://docs.mongodb.org/master/reference/command/find/>`_ in the MongoDB Manual.

The "explain" command
---------------------

With MongoDB before 3.2, a query with option ``$explain: true`` returns information about the query plan, instead of the query results. Beginning in MongoDB 3.2, there is a separate "explain" command. The driver will not convert "$explain" queries to "explain" commands, you must call the "explain" command explicitly:

.. code-block:: c

  /* MongoDB 3.2+, "explain" command syntax */
  command = BCON_NEW ("explain",
                      "{",
                      "find",
                      BCON_UTF8 ("collection_name"),
                      "filter",
                      "{",
                      "foo",
                      BCON_INT32 (1),
                      "}",
                      "}");
  mongoc_collection_command_simple (collection, command, NULL, &reply, &error);

See Also
--------

`The "explain" command <https://docs.mongodb.org/master/reference/command/explain/>`_ in the MongoDB Manual.

