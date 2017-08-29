:man_page: mongoc_collection_create_bulk_operation

mongoc_collection_create_bulk_operation()
=========================================

Synopsis
--------

.. code-block:: c

  mongoc_bulk_operation_t *
  mongoc_collection_create_bulk_operation (
     mongoc_collection_t *collection,
     bool ordered,
     const mongoc_write_concern_t *write_concern) BSON_GNUC_WARN_UNUSED_RESULT;

Parameters
----------

* ``collection``: A :symbol:`mongoc_collection_t`.
* ``ordered``: If the operations must be performed in order.
* ``write_concern``: An optional :symbol:`mongoc_write_concern_t` or ``NULL``.

Description
-----------

This function shall begin a new bulk operation. After creating this you may call various functions such as :symbol:`mongoc_bulk_operation_update()`, :symbol:`mongoc_bulk_operation_insert()` and others.

After calling :symbol:`mongoc_bulk_operation_execute()` the commands will be executed in as large as batches as reasonable by the client.

If ``ordered`` is true, then processing will stop at the first error.

If ``ordered`` is not true, then the bulk operation will attempt to continue processing even after the first failure.

``write_concern`` contains the write concern for all operations in the bulk operation. If ``NULL``, the collection's write concern is used. The global default is acknowledged writes: MONGOC_WRITE_CONCERN_W_DEFAULT.

See Also
--------

:symbol:`Bulk Write Operations <bulk>`

:symbol:`mongoc_bulk_operation_t`

Errors
------

Errors are propagated when executing the bulk operation.

Returns
-------

A newly allocated :symbol:`mongoc_bulk_operation_t` that should be freed with :symbol:`mongoc_bulk_operation_destroy()` when no longer in use.

.. warning::

  Failure to handle the result of this function is a programming error.

