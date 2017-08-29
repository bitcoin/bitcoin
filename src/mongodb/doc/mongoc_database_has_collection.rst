:man_page: mongoc_database_has_collection

mongoc_database_has_collection()
================================

Synopsis
--------

.. code-block:: c

  bool
  mongoc_database_has_collection (mongoc_database_t *database,
                                  const char *name,
                                  bson_error_t *error);

This function checks to see if a collection exists on the MongoDB server within ``database``.

Parameters
----------

* ``database``: A :symbol:`mongoc_database_t`.
* ``name``: A string containing the name of the collection.
* ``error``: An optional location for a :symbol:`bson_error_t <errors>` or ``NULL``.

Errors
------

Errors are propagated via the ``error`` parameter.

Returns
-------

If the function succeeds, it returns ``true`` if the collection exists and ``false`` if not, and in either case the fields of ``error`` are cleared, if ``error`` is not NULL.

Returns ``false`` and sets ``error`` if there are invalid arguments or a server or network error.
