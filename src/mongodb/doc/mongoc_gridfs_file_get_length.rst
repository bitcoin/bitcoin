:man_page: mongoc_gridfs_file_get_length

mongoc_gridfs_file_get_length()
===============================

Synopsis
--------

.. code-block:: c

  int64_t
  mongoc_gridfs_file_get_length (mongoc_gridfs_file_t *file);

Parameters
----------

* ``file``: A :symbol:`mongoc_gridfs_file_t`.

Description
-----------

Fetches the length of the gridfs file in bytes.

Returns
-------

A 64-bit signed integer.

