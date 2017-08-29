:man_page: mongoc_bulk_operation_destroy

mongoc_bulk_operation_destroy()
===============================

Synopsis
--------

.. code-block:: c

  void
  mongoc_bulk_operation_destroy (mongoc_bulk_operation_t *bulk);

Destroys a :symbol:`mongoc_bulk_operation_t` and frees the structure.

Parameters
----------

* ``bulk``: A :symbol:`mongoc_bulk_operation_t`.

