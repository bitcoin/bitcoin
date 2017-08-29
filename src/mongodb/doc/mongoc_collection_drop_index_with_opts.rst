:man_page: mongoc_collection_drop_index_with_opts

mongoc_collection_drop_index_with_opts()
========================================

Synopsis
--------

.. code-block:: c

  bool
  mongoc_collection_drop_index_with_opts (mongoc_collection_t *collection,
                                          const char *index_name,
                                          const bson_t *opts,
                                          bson_error_t *error);

Parameters
----------

* ``collection``: A :symbol:`mongoc_collection_t`.
* ``index_name``: A string containing the name of the index.
* ``opts``: A :symbol:`bson:bson_t` or ``NULL``.
* ``error``: An optional location for a :symbol:`bson_error_t <errors>` or ``NULL``.

Description
-----------

This function requests than an index on ``collection`` be dropped.

If no write concern is provided in ``opts``, the collection's write concern is used.

Errors
------

Errors are propagated via the ``error`` parameter.

Returns
-------

Returns ``true`` if successful. Returns ``false`` and sets ``error`` if there are invalid arguments or a server or network error.
