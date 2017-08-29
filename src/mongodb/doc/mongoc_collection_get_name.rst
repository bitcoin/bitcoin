:man_page: mongoc_collection_get_name

mongoc_collection_get_name()
============================

Synopsis
--------

.. code-block:: c

  const char *
  mongoc_collection_get_name (mongoc_collection_t *collection);

Parameters
----------

* ``collection``: A :symbol:`mongoc_collection_t`.

Description
-----------

Fetches the name of ``collection``.

Returns
-------

A string which should not be modified or freed.

