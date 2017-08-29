:man_page: mongoc_collection_drop

mongoc_collection_drop()
========================

Synopsis
--------

.. code-block:: c

  bool
  mongoc_collection_drop (mongoc_collection_t *collection, bson_error_t *error);

Parameters
----------

* ``collection``: A :symbol:`mongoc_collection_t`.
* ``error``: An optional location for a :symbol:`bson_error_t <errors>` or ``NULL``.

Description
-----------

For more information, see :symbol:`mongoc_collection_drop_with_opts()`. This function is a thin wrapper, passing ``NULL`` in as :symbol:`opts <bson:bson_t>` parameter.

