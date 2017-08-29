:man_page: mongoc_gridfs_file_get_content_type

mongoc_gridfs_file_get_content_type()
=====================================

Synopsis
--------

.. code-block:: c

  const char *
  mongoc_gridfs_file_get_content_type (mongoc_gridfs_file_t *file);

Parameters
----------

* ``file``: A :symbol:`mongoc_gridfs_file_t`.

Description
-----------

Fetches the content type specified for the underlying file.

Returns
-------

Returns a string which should not be modified or freed.

