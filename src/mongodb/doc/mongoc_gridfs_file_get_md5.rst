:man_page: mongoc_gridfs_file_get_md5

mongoc_gridfs_file_get_md5()
============================

Synopsis
--------

.. code-block:: c

  const char *
  mongoc_gridfs_file_get_md5 (mongoc_gridfs_file_t *file);

Parameters
----------

* ``file``: A :symbol:`mongoc_gridfs_file_t`.

Description
-----------

Fetches the pre-computed MD5 for the underlying gridfs file.

Returns
-------

Returns a string that should not be modified or freed.

