:man_page: mongoc_bulk_operation_replace_one

mongoc_bulk_operation_replace_one()
===================================

Synopsis
--------

.. code-block:: c

  void
  mongoc_bulk_operation_replace_one (mongoc_bulk_operation_t *bulk,
                                     const bson_t *selector,
                                     const bson_t *document,
                                     bool upsert);

Replace a single document as part of a bulk operation. This only queues the operation. To execute it, call :symbol:`mongoc_bulk_operation_execute()`.

This function is superseded by :symbol:`mongoc_bulk_operation_replace_one_with_opts()`.

Parameters
----------

* ``bulk``: A :symbol:`mongoc_bulk_operation_t`.
* ``selector``: A :symbol:`bson:bson_t` that selects which document to remove.
* ``document``: A :symbol:`bson:bson_t` containing the replacement document.
* ``upsert``: ``true`` if this should be an ``upsert``.

.. warning::

  ``document`` may not contain fields with keys containing ``.`` or ``$``.

See Also
--------

:symbol:`mongoc_bulk_operation_replace_one_with_opts()`

Errors
------

Errors are propagated via :symbol:`mongoc_bulk_operation_execute()`.

