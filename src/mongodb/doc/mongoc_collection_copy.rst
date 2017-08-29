:man_page: mongoc_collection_copy

mongoc_collection_copy()
========================

Synopsis
--------

.. code-block:: c

  mongoc_collection_t *
  mongoc_collection_copy (mongoc_collection_t *collection);

Parameters
----------

* ``collection``: A :symbol:`mongoc_collection_t`.

Description
-----------

Performs a deep copy of the ``collection`` struct and its configuration. Useful if you intend to call :symbol:`mongoc_collection_set_write_concern`, :symbol:`mongoc_collection_set_read_prefs`, or :symbol:`mongoc_collection_set_read_concern`, and want to preserve an unaltered copy of the struct.

This function does *not* copy the contents of the collection on the MongoDB server; use the :ref:`cloneCollection command <mongoc-common-task-examples_clone_collection>` for that purpose.

Returns
-------

A newly allocated :symbol:`mongoc_collection_t` that should be freed with :symbol:`mongoc_collection_destroy()` when no longer in use.

