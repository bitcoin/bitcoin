:man_page: mongoc_gridfs_file_set_filename

mongoc_gridfs_file_set_filename()
=================================

Synopsis
--------

.. code-block:: c

  void
  mongoc_gridfs_file_set_filename (mongoc_gridfs_file_t *file,
                                   const char *filename);

Parameters
----------

* ``file``: A :symbol:`mongoc_gridfs_file_t`.
* ``str``: A UTF-8 encoded string containing the filename.

Description
-----------

Sets the filename for ``file``.

You need to call :symbol:`mongoc_gridfs_file_save()` to persist this change.

