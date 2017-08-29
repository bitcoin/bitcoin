:man_page: mongoc_matcher_t

mongoc_matcher_t
================

Client-side document matching abstraction

Synopsis
--------

.. code-block:: c

  typedef struct _mongoc_matcher_t mongoc_matcher_t;

``mongoc_matcher_t`` provides a reduced-interface for client-side matching of BSON documents.

It can perform the basics such as $in, $nin, $eq, $neq, $gt, $gte, $lt, and $lte.

.. warning::

  ``mongoc_matcher_t`` does not currently support the full spectrum of query operations that the MongoDB server supports.

Deprecated
----------

.. warning::

  ``mongoc_matcher_t`` is deprecated and will be removed in version 2.0.

.. only:: html

  Functions
  ---------

  .. toctree::
    :titlesonly:
    :maxdepth: 1

    mongoc_matcher_destroy
    mongoc_matcher_match
    mongoc_matcher_new

Example
-------

.. code-block:: c
  :caption: Filter a sequence of BSON documents from STDIN based on a query

  #include <bcon.h>
  #include <bson.h>
  #include <mongoc.h>
  #include <stdio.h>

  int
  main (int argc, char *argv[])
  {
     mongoc_matcher_t *matcher;
     bson_reader_t *reader;
     const bson_t *bson;
     bson_t *spec;
     char *str;
     int fd;

     mongoc_init ();

  #ifdef _WIN32
     fd = fileno (stdin);
  #else
     fd = STDIN_FILENO;
  #endif

     reader = bson_reader_new_from_fd (fd, false);

     spec = BCON_NEW ("hello", "world");
     matcher = mongoc_matcher_new (spec, NULL);

     while ((bson = bson_reader_read (reader, NULL))) {
        if (mongoc_matcher_match (matcher, bson)) {
           str = bson_as_canonical_extended_json (bson, NULL);
           printf ("%s\n", str);
           bson_free (str);
        }
     }

     bson_reader_destroy (reader);
     bson_destroy (spec);

     mongoc_cleanup ();

     return 0;
  }

