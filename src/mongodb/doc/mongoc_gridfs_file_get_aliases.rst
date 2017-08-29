:man_page: mongoc_gridfs_file_get_aliases

mongoc_gridfs_file_get_aliases()
================================

Synopsis
--------

.. code-block:: c

  const bson_t *
  mongoc_gridfs_file_get_aliases (mongoc_gridfs_file_t *file);

Parameters
----------

* ``file``: A :symbol:`mongoc_gridfs_file_t`.

Description
-----------

Fetches the aliases associated with a gridfs file.

Returns
-------

Returns a :symbol:`const bson_t * <bson:bson_t>` that should not be modified or freed.

