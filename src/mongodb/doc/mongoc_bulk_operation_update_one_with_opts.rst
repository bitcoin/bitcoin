:man_page: mongoc_bulk_operation_update_one_with_opts

mongoc_bulk_operation_update_one_with_opts()
============================================

Synopsis
--------

.. code-block:: c

  bool
  mongoc_bulk_operation_update_one_with_opts (mongoc_bulk_operation_t *bulk,
                                              const bson_t *selector,
                                              const bson_t *document,
                                              const bson_t *opts,
                                              bson_error_t *error); /* OUT */

This function queues an update as part of a bulk operation. It will only modify a single document on the MongoDB server. This function does not execute the operation. To execute the entirety of the bulk operation call :symbol:`mongoc_bulk_operation_execute()`.

Parameters
----------

* ``bulk``: A :symbol:`mongoc_bulk_operation_t`.
* ``selector``: A :symbol:`bson:bson_t` that selects which document to remove.
* ``document``: A :symbol:`bson:bson_t` containing the update document.
* ``opts``: A :symbol:`bson:bson_t` containing additional options.
* ``error``: A :symbol:`bson:bson_error_t` any errors that may have occurred.

.. warning::

  ``document`` *must only* contain fields whose key starts with ``$``. See the update document specification for more details.

See Also
--------

:symbol:`mongoc_bulk_operation_update_many_with_opts()`

Errors
------

Operation errors are propagated via :symbol:`mongoc_bulk_operation_execute()`, while argument validation errors are reported by the ``error`` argument.

Returns
-------

Returns true on success, and false if there is a server or network error or if passed invalid arguments.

