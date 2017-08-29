:man_page: mongoc_collection_drop_index

mongoc_collection_drop_index()
==============================

Synopsis
--------

.. code-block:: c

  bool
  mongoc_collection_drop_index (mongoc_collection_t *collection,
                                const char *index_name,
                                bson_error_t *error);

Parameters
----------

* ``collection``: A :symbol:`mongoc_collection_t`.
* ``index_name``: A string containing the name of the index.
* ``error``: An optional location for a :symbol:`bson_error_t <errors>` or ``NULL``.

Description
-----------

For more information, see :symbol:`mongoc_collection_drop_with_opts() <mongoc_collection_drop_index_with_opts>`. This function is a thin wrapper, passing ``NULL`` in as :symbol:`opts <bson:bson_t>` parameter.

