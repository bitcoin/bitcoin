:man_page: mongoc_collection_delete

mongoc_collection_delete()
==========================

Synopsis
--------

.. code-block:: c

  bool
  mongoc_collection_delete (mongoc_collection_t *collection,
                            mongoc_delete_flags_t flags,
                            const bson_t *selector,
                            const mongoc_write_concern_t *write_concern,
                            bson_error_t *error);

Deprecated
----------

.. warning::

  This function is deprecated and should not be used in new code.

Please use :symbol:`mongoc_collection_remove()` instead.

Parameters
----------

* ``collection``: A :symbol:`mongoc_collection_t`.
* ``flags``: A :symbol:`mongoc_delete_flags_t`.
* ``selector``: A :symbol:`bson:bson_t` containing the query to match documents.
* ``write_concern``: A :symbol:`mongoc_write_concern_t` or ``NULL``.
* ``error``: An optional location for a :symbol:`bson_error_t <errors>` or ``NULL``.

Description
-----------

This function shall delete documents in the given ``collection`` that match ``selector``. The bson ``selector`` is not validated, simply passed along as appropriate to the server.  As such, compatibility and errors should be validated in the appropriate server documentation.

If you want to limit deletes to a single document, provide ``MONGOC_DELETE_SINGLE_REMOVE`` in ``flags``.

Errors
------

Errors are propagated via the ``error`` parameter.

Returns
-------

Returns ``true`` if successful. Returns ``false`` and sets ``error`` if there are invalid arguments or a server or network error.

A write concern timeout or write concern error is considered a failure.

