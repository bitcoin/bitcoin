:man_page: mongoc_gridfs_file_list_next

mongoc_gridfs_file_list_next()
==============================

Synopsis
--------

.. code-block:: c

  mongoc_gridfs_file_t *
  mongoc_gridfs_file_list_next (mongoc_gridfs_file_list_t *list);

Parameters
----------

* ``list``: A :symbol:`mongoc_gridfs_file_list_t`.

Description
-----------

This function shall iterate the underlying gridfs file list, returning the next file each iteration. This is a blocking function.

Returns
-------

A newly allocated :symbol:`mongoc_gridfs_file_t` that should be freed with :symbol:`mongoc_gridfs_file_destroy()` when no longer in use.

