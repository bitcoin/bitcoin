:man_page: mongoc_gridfs_file_get_upload_date

mongoc_gridfs_file_get_upload_date()
====================================

Synopsis
--------

.. code-block:: c

  int64_t
  mongoc_gridfs_file_get_upload_date (mongoc_gridfs_file_t *file);

Parameters
----------

* ``file``: A :symbol:`mongoc_gridfs_file_t`.

Description
-----------

Fetches the specified upload date of the gridfs file in milliseconds since the UNIX epoch.

Returns
-------

A signed int64 with the upload date in milliseconds since the UNIX epoch.

