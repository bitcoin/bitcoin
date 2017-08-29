:man_page: mongoc_gridfs_file_get_chunk_size

mongoc_gridfs_file_get_chunk_size()
===================================

Synopsis
--------

.. code-block:: c

  int32_t
  mongoc_gridfs_file_get_chunk_size (mongoc_gridfs_file_t *file);

Parameters
----------

* ``file``: A :symbol:`mongoc_gridfs_file_t`.

Description
-----------

Fetches the chunk size used with the underlying gridfs file.

Returns
-------

A signed 32-bit integer.

