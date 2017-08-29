:man_page: mongoc_collection_destroy

mongoc_collection_destroy()
===========================

Synopsis
--------

.. code-block:: c

  void
  mongoc_collection_destroy (mongoc_collection_t *collection);

Parameters
----------

* ``collection``: A :symbol:`mongoc_collection_t`.

Description
-----------

This function shall destroy a :symbol:`mongoc_collection_t` and its associated resources.

.. warning::

  Always destroy a :symbol:`mongoc_cursor_t` created from a collection before destroying the collection.

