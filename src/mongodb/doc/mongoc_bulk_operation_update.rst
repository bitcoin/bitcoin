:man_page: mongoc_bulk_operation_update

mongoc_bulk_operation_update()
==============================

Synopsis
--------

.. code-block:: c

  void
  mongoc_bulk_operation_update (mongoc_bulk_operation_t *bulk,
                                const bson_t *selector,
                                const bson_t *document,
                                bool upsert);

This function queues an update as part of a bulk operation. This does not execute the operation. To execute the entirety of the bulk operation call :symbol:`mongoc_bulk_operation_execute()`.

``document`` MUST only contain fields whose key starts with ``$``. See the update document specification for more details.

This function is superseded by :symbol:`mongoc_bulk_operation_update_one_with_opts()` and :symbol:`mongoc_bulk_operation_update_many_with_opts()`.

Parameters
----------

* ``bulk``: A :symbol:`mongoc_bulk_operation_t`.
* ``selector``: A :symbol:`bson:bson_t` that selects which documents to remove.
* ``document``: A :symbol:`bson:bson_t` containing the update document.
* ``upsert``: ``true`` if an ``upsert`` should be performed.

See Also
--------

:symbol:`mongoc_bulk_operation_update_one_with_opts()`

:symbol:`mongoc_bulk_operation_update_many_with_opts()`

Errors
------

Errors are propagated via :symbol:`mongoc_bulk_operation_execute()`.

