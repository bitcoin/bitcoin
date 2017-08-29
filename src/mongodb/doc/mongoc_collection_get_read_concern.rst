:man_page: mongoc_collection_get_read_concern

mongoc_collection_get_read_concern()
====================================

Synopsis
--------

.. code-block:: c

  const mongoc_read_concern_t *
  mongoc_collection_get_read_concern (const mongoc_collection_t *collection);

Parameters
----------

* ``collection``: A :symbol:`mongoc_collection_t`.

Description
-----------

Fetches the default read concern to be used on read operations originating from ``collection``.

Returns
-------

A :symbol:`mongoc_read_concern_t` that should not be modified or freed.

