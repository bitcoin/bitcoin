:man_page: mongoc_index_opt_t

mongoc_index_opt_t
==================

Synopsis
--------

.. code-block:: c

  #include <mongoc.h>

  typedef struct {
     bool is_initialized;
     bool background;
     bool unique;
     const char *name;
     bool drop_dups;
     bool sparse;
     int32_t expire_after_seconds;
     int32_t v;
     const bson_t *weights;
     const char *default_language;
     const char *language_override;
     mongoc_index_opt_geo_t *geo_options;
     mongoc_index_opt_storage_t *storage_options;
     const bson_t *partial_filter_expression;
     const bson_t *collation;
     void *padding[4];
  } mongoc_index_opt_t;

Deprecated
----------

This structure is deprecated and should not be used in new code. See :doc:`create-indexes`.

Description
-----------

This structure contains the options that may be used for tuning a specific index.

See the `createIndexes documentations <https://docs.mongodb.org/manual/reference/command/createIndexes/>`_ in the MongoDB manual for descriptions of individual options.

.. note::

   dropDups is deprecated as of MongoDB version 3.0.0.  This option is silently ignored by the server and unique index builds using this option will fail if a duplicate value is detected.

Example
-------

.. code-block:: c

  {
     bson_t keys;
     bson_error_t error;
     mongoc_index_opt_t opt;
     mongoc_index_opt_geo_t geo_opt;

     mongoc_index_opt_init (&opt);
     mongoc_index_opt_geo_init (&geo_opt);

     bson_init (&keys);
     BSON_APPEND_UTF8 (&keys, "location", "2d");

     geo_opt.twod_location_min = -123;
     geo_opt.twod_location_max = +123;
     geo_opt.twod_bits_precision = 30;
     opt.geo_options = &geo_opt;

     collection = mongoc_client_get_collection (client, "test", "geo_test");
     if (mongoc_collection_create_index (collection, &keys, &opt, &error)) {
        /* Successfully created the geo index */
     }
     bson_destroy (&keys);
     mongoc_collection_destroy (&collection);
  }

.. only:: html

  Functions
  ---------

  .. toctree::
    :titlesonly:
    :maxdepth: 1

    mongoc_index_opt_get_default
    mongoc_index_opt_init

See Also
--------

:doc:`mongoc_index_opt_geo_t`

:doc:`mongoc_index_opt_wt_t`

