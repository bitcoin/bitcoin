:man_page: mongoc_database_create_collection

mongoc_database_create_collection()
===================================

Synopsis
--------

.. code-block:: c

  mongoc_collection_t *
  mongoc_database_create_collection (mongoc_database_t *database,
                                     const char *name,
                                     const bson_t *opts,
                                     bson_error_t *error);

Parameters
----------

* ``database``: A :symbol:`mongoc_database_t`.
* ``name``: The name of the new collection.
* ``opts``: An optional :symbol:`bson:bson_t` for opts to the ``createDatabase`` command.
* ``error``: A location for a :symbol:`bson_error_t <errors>` or ``NULL``.

Description
-----------

This function creates a :symbol:`mongoc_collection_t` from the given :symbol:`mongoc_database_t`.

If no write concern is provided in ``opts``, the database's write concern is used.

Errors
------

Errors are propagated via the ``error`` parameter.

Returns
-------

This function returns a newly allocated :symbol:`mongoc_collection_t` upon success, ``NULL`` upon failure and ``error`` is set.

