:man_page: mongoc_collection_insert_bulk

mongoc_collection_insert_bulk()
===============================

Synopsis
--------

.. code-block:: c

  bool
  mongoc_collection_insert_bulk (mongoc_collection_t *collection,
                                 mongoc_insert_flags_t flags,
                                 const bson_t **documents,
                                 uint32_t n_documents,
                                 const mongoc_write_concern_t *write_concern,
                                 bson_error_t *error)
     BSON_GNUC_DEPRECATED_FOR (mongoc_collection_create_bulk_operation);

Deprecated
----------

This function is deprecated and should not be used in new code.

Please use :symbol:`mongoc_collection_create_bulk_operation()` instead.

Parameters
----------

* ``collection``: A :symbol:`mongoc_collection_t`.
* ``flags``: A bitwise or of :symbol:`mongoc_insert_flags_t`.
* ``documents``: An array of :symbol:`const bson_t * <bson:bson_t>`.
* ``n_documents``: The number of documents in ``documents``.
* ``write_concern``: A :symbol:`mongoc_write_concern_t` or ``NULL``.
* ``error``: An optional location for a :symbol:`bson_error_t <errors>` or ``NULL``.

Description
-----------

This function performs a bulk insert of all of the documents in ``documents``. This function is deprecated as it cannot accurately return which documents may have failed during the bulk insert.

Errors
------

Errors are propagated via the ``error`` parameter.

A write concern timeout or write concern error is considered a failure.

Returns
-------

Returns ``true`` if successful. Returns ``false`` and sets ``error`` if there are invalid arguments or a server or network error.

