:man_page: mongoc_bulk_operation_delete_one

mongoc_bulk_operation_delete_one()
==================================

Synopsis
--------

.. code-block:: c

  void
  mongoc_bulk_operation_delete_one (mongoc_bulk_operation_t *bulk,
                                    const bson_t *selector);

Delete a single document as part of a bulk operation. This only queues the operation. To execute it, call :symbol:`mongoc_bulk_operation_execute()`.

Deprecated
----------

.. warning::

  This function is deprecated and should not be used in new code.

Please use :symbol:`mongoc_bulk_operation_remove_one()` instead.

Parameters
----------

* ``bulk``: A :symbol:`mongoc_bulk_operation_t`.
* ``selector``: A :symbol:`bson:bson_t`.

See Also
--------

:symbol:`mongoc_bulk_operation_remove_one_with_opts()`

:symbol:`mongoc_bulk_operation_remove_many_with_opts()`

Errors
------

Errors are propagated via :symbol:`mongoc_bulk_operation_execute()`.

