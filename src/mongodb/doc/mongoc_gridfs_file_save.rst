:man_page: mongoc_gridfs_file_save

mongoc_gridfs_file_save()
=========================

Synopsis
--------

.. code-block:: c

  bool
  mongoc_gridfs_file_save (mongoc_gridfs_file_t *file);

Parameters
----------

* ``file``: A :symbol:`mongoc_gridfs_file_t`.

Description
-----------

Saves modifications to ``file`` to the MongoDB server.

If an error occurred, false is returned and the error can be retrieved with :symbol:`mongoc_gridfs_file_error()`.

Returns
-------

Returns true if successful, otherwise false.

