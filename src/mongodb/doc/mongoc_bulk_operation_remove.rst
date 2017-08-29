:man_page: mongoc_bulk_operation_remove

mongoc_bulk_operation_remove()
==============================

Synopsis
--------

.. code-block:: c

  void
  mongoc_bulk_operation_remove (mongoc_bulk_operation_t *bulk,
                                const bson_t *selector);

Remove documents as part of a bulk operation. This only queues the operation. To execute it, call :symbol:`mongoc_bulk_operation_execute()`.

This function is superseded by :symbol:`mongoc_bulk_operation_remove_one_with_opts()` and :symbol:`mongoc_bulk_operation_remove_many_with_opts()`.

Parameters
----------

* ``bulk``: A :symbol:`mongoc_bulk_operation_t`.
* ``selector``: A :symbol:`bson:bson_t`.

See Also
--------

:symbol:`mongoc_bulk_operation_remove_one()`

:symbol:`mongoc_bulk_operation_remove_one_with_opts()`

:symbol:`mongoc_bulk_operation_remove_many_with_opts()`

Errors
------

Errors are propagated via :symbol:`mongoc_bulk_operation_execute()`.

