:man_page: mongoc_database_find_collections

mongoc_database_find_collections()
==================================

Synopsis
--------

.. code-block:: c

  mongoc_cursor_t *
  mongoc_database_find_collections (mongoc_database_t *database,
                                    const bson_t *filter,
                                    bson_error_t *error);

Fetches a cursor containing documents, each corresponding to a collection on this database.

Parameters
----------

* ``database``: A :symbol:`mongoc_database_t`.
* ``filter``: A matcher used by the server to filter the returned collections. May be ``NULL``.
* ``error``: An optional location for a :symbol:`bson_error_t <errors>` or ``NULL``.

Errors
------

Errors are propagated via the ``error`` parameter.

Returns
-------

A cursor where each result corresponds to the server's representation of a collection in this database.

