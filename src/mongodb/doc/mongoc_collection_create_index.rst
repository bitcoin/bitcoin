:man_page: mongoc_collection_create_index

mongoc_collection_create_index()
================================

Synopsis
--------

.. code-block:: c

  bool
  mongoc_collection_create_index (mongoc_collection_t *collection,
                                  const bson_t *keys,
                                  const mongoc_index_opt_t *opt,
                                  bson_error_t *error);

Deprecated
----------

This function is deprecated and should not be used in new code. See :doc:`create-indexes`.

Parameters
----------

* ``collection``: A :symbol:`mongoc_collection_t`.
* ``keys``: A :symbol:`bson:bson_t`.
* ``opt``: A mongoc_index_opt_t.
* ``error``: An optional location for a :symbol:`bson_error_t <errors>` or ``NULL``.
