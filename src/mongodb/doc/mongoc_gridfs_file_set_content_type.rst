:man_page: mongoc_gridfs_file_set_content_type

mongoc_gridfs_file_set_content_type()
=====================================

Synopsis
--------

.. code-block:: c

  void
  mongoc_gridfs_file_set_content_type (mongoc_gridfs_file_t *file,
                                       const char *content_type);

Parameters
----------

* ``file``: A :symbol:`mongoc_gridfs_file_t`.
* ``str``: A string containing the content type.

Description
-----------

Sets the content type for the gridfs file. This should be something like ``"text/plain"``.

You need to call :symbol:`mongoc_gridfs_file_save()` to persist this change.

