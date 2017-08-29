:man_page: mongoc_gridfs_file_set_md5

mongoc_gridfs_file_set_md5()
============================

Synopsis
--------

.. code-block:: c

  void
  mongoc_gridfs_file_set_md5 (mongoc_gridfs_file_t *file, const char *md5);

Parameters
----------

* ``file``: A :symbol:`mongoc_gridfs_file_t`.
* ``str``: A string containing the MD5 of the file.

Description
-----------

Sets the MD5 checksum for ``file``.

You need to call :symbol:`mongoc_gridfs_file_save()` to persist this change.

