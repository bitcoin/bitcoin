:man_page: mongoc_collection_get_write_concern

mongoc_collection_get_write_concern()
=====================================

Synopsis
--------

.. code-block:: c

  const mongoc_write_concern_t *
  mongoc_collection_get_write_concern (const mongoc_collection_t *collection);

Parameters
----------

* ``collection``: A :symbol:`mongoc_collection_t`.

Description
-----------

Fetches the default write concern to be used on write operations originating from ``collection`` and not specifying a write concern.

Returns
-------

A :symbol:`mongoc_write_concern_t` that should not be modified or freed.

