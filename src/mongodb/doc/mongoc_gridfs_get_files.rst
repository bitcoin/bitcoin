:man_page: mongoc_gridfs_get_files

mongoc_gridfs_get_files()
=========================

Synopsis
--------

.. code-block:: c

  mongoc_collection_t *
  mongoc_gridfs_get_files (mongoc_gridfs_t *gridfs);

Parameters
----------

* ``gridfs``: A :symbol:`mongoc_gridfs_t`.

Description
-----------

Retrieves the :symbol:`mongoc_collection_t` containing the file metadata for GridFS. This instance is owned by the :symbol:`mongoc_gridfs_t` and should not be modified or freed.

Returns
-------

Returns a :symbol:`mongoc_collection_t` that should not be modified or freed.

