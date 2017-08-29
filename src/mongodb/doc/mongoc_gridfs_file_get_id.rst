:man_page: mongoc_gridfs_file_get_id

mongoc_gridfs_file_get_id()
===========================

Synopsis
--------

.. code-block:: c

  const bson_value_t *
  mongoc_gridfs_file_get_id (mongoc_gridfs_file_t *file);

Parameters
----------

* ``file``: A :symbol:`mongoc_gridfs_file_t`.

Description
-----------

Fetches the id of a gridfs file.

The C Driver always uses an ObjectId for ``_id``, but files created by other drivers may have any type of ``_id``.

Returns
-------

Returns a :symbol:`const bson_value_t * <bson:bson_value_t>` that should not be modified or freed.

