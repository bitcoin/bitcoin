:man_page: mongoc_collection_get_last_error

mongoc_collection_get_last_error()
==================================

Synopsis
--------

.. code-block:: c

  const bson_t *
  mongoc_collection_get_last_error (const mongoc_collection_t *collection);

Parameters
----------

* ``collection``: A :symbol:`mongoc_collection_t`.

Description
-----------

The mongoc_collection_get_last_error() function returns a bulk result. See :doc:`Bulk Write Operations <bulk>` for examples of bulk results.

A write_concern must be at least ``MONGOC_WRITE_CONCERN_W_DEFAULT`` in last command execution for this to be available.

Returns
-------

A :symbol:`bson:bson_t` that should not be modified or ``NULL``.

