:man_page: mongoc_gridfs_file_tell

mongoc_gridfs_file_tell()
=========================

Synopsis
--------

.. code-block:: c

  uint64_t
  mongoc_gridfs_file_tell (mongoc_gridfs_file_t *file);

Parameters
----------

* ``file``: A :symbol:`mongoc_gridfs_file_t`.

Description
-----------

This function returns the current position indicator within ``file``.

Returns
-------

Returns a file position as an unsigned 64-bit integer.

