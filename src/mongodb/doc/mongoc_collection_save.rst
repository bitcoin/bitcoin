:man_page: mongoc_collection_save

mongoc_collection_save()
========================

Deprecated
----------

This function is deprecated and should not be used in new code.

Please use :symbol:`mongoc_collection_insert()` or
:symbol:`mongoc_collection_update()` instead.


Synopsis
--------

.. code-block:: c

  bool
  mongoc_collection_save (mongoc_collection_t *collection,
                          const bson_t *document,
                          const mongoc_write_concern_t *write_concern,
                          bson_error_t *error)
      BSON_GNUC_DEPRECATED_FOR (mongoc_collection_insert or mongoc_collection_update);

Parameters
----------

* ``collection``: A :symbol:`mongoc_collection_t`.
* ``document``: A :symbol:`bson:bson_t` containing the document.
* ``write_concern``: A :symbol:`mongoc_write_concern_t` or ``NULL`` for default write concern.
* ``error``: An optional location for a :symbol:`bson_error_t <errors>` or ``NULL``.

Description
-----------

This function shall save a document into ``collection``. If the document has an ``_id`` field it will be updated. Otherwise it will be inserted.

Errors
------

Errors are propagated via the ``error`` parameter.

Returns
-------

Returns ``true`` if successful. Returns ``false`` and sets ``error`` if there are invalid arguments or a server or network error.

A write concern timeout or write concern error is considered a failure.

