:man_page: mongoc_collection_count_with_opts

mongoc_collection_count_with_opts()
===================================

Synopsis
--------

.. code-block:: c

  int64_t
  mongoc_collection_count_with_opts (mongoc_collection_t *collection,
                                     mongoc_query_flags_t flags,
                                     const bson_t *query,
                                     int64_t skip,
                                     int64_t limit,
                                     const bson_t *opts,
                                     const mongoc_read_prefs_t *read_prefs,
                                     bson_error_t *error);

Parameters
----------

* ``collection``: A :symbol:`mongoc_collection_t`.
* ``flags``: A :symbol:`mongoc_query_flags_t`.
* ``query``: A :symbol:`bson:bson_t` containing the query.
* ``skip``: A int64_t, zero to ignore.
* ``limit``: A int64_t, zero to ignore.
* ``opts``: A :symbol:`bson:bson_t`, ``NULL`` to ignore.
* ``read_prefs``: An optional :symbol:`mongoc_read_prefs_t`, otherwise uses the collection's read preference.
* ``error``: An optional location for a :symbol:`bson_error_t <errors>` or ``NULL``.

Description
-----------

This function shall execute a count query on the underlying 'collection'. The bson 'query' is not validated, simply passed along as appropriate to the server.  As such, compatibility and errors should be validated in the appropriate server documentation.

The :symbol:`mongoc_read_concern_t` specified on the :symbol:`mongoc_collection_t` will be used, if any. If ``read_prefs`` is NULL, the collection's read preferences are used.

In addition to the standard functionality available from mongoc_collection_count, this function allows the user to add arbitrary extra keys to the count.  This pass through enables features such as hinting for counts.

For more information, see the `query reference <http://docs.mongodb.org/manual/reference/operator/query/>`_ at the MongoDB website.

Errors
------

Errors are propagated via the ``error`` parameter.

Returns
-------

-1 on failure, otherwise the number of documents counted.

Examples
--------

.. code-block:: c
  :caption: Basic Counting

  #include <mongoc.h>
  #include <bcon.h>
  #include <stdio.h>

  static void
  print_query_count (mongoc_collection_t *collection, bson_t *query)
  {
     bson_error_t error;
     int64_t count;
     bson_t opts;

     bson_init (&opts);
     BSON_APPEND_UTF8 (&opts, "hint", "_id_");

     count = mongoc_collection_count_with_opts (
        collection, MONGOC_QUERY_NONE, query, 0, 0, &opts, NULL, &error);

     bson_destroy (&opts);

     if (count < 0) {
        fprintf (stderr, "Count failed: %s\n", error.message);
     } else {
        printf ("%" PRId64 " documents counted.\n", count);
     }
  }

.. code-block:: c
  :caption: Counting with Collation

  #include <mongoc.h>
  #include <bcon.h>
  #include <stdio.h>

  static void
  print_query_count (mongoc_collection_t *collection, bson_t *query)
  {
     bson_t *selector;
     bson_t *opts;
     bson_error_t error;
     int64_t count;

     selector = BCON_NEW ("_id", "{", "$gt", BCON_UTF8 ("one"), "}");

     /* "One" normally sorts before "one"; make "one" come first */
     opts = BCON_NEW ("collation",
                      "{",
                      "locale",
                      BCON_UTF8 ("en_US"),
                      "caseFirst",
                      BCON_UTF8 ("lower"),
                      "}");

     count = mongoc_collection_count_with_opts (
        collection, MONGOC_QUERY_NONE, query, 0, 0, opts, NULL, &error);

     bson_destroy (selector);
     bson_destroy (opts);

     if (count < 0) {
        fprintf (stderr, "Count failed: %s\n", error.message);
     } else {
        printf ("%" PRId64 " documents counted.\n", count);
     }
  }

