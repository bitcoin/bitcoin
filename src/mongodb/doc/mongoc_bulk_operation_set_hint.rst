:man_page: mongoc_bulk_operation_set_hint

mongoc_bulk_operation_set_hint()
================================

Synopsis
--------

.. code-block:: c

  void
  mongoc_bulk_operation_set_hint (const mongoc_bulk_operation_t *bulk,
                                  uint32_t server_id);

Parameters
----------

* ``bulk``: A :symbol:`mongoc_bulk_operation_t`.
* ``server_id``: An opaque id identifying the server to use.

Description
-----------

Specifies which server to use for the operation. This function has an effect only if called before :symbol:`mongoc_bulk_operation_execute`.

(The function name includes the old term "hint" for the sake of backward compatibility, but we now call this number a "server id".)

Use ``mongoc_bulk_operation_set_hint`` only for building a language driver that wraps the C Driver. When writing applications in C, leave the server id unset and allow the driver to choose a suitable server for the bulk operation.

