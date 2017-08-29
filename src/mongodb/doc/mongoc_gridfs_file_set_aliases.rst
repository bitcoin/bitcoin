:man_page: mongoc_gridfs_file_set_aliases

mongoc_gridfs_file_set_aliases()
================================

Synopsis
--------

.. code-block:: c

  void
  mongoc_gridfs_file_set_aliases (mongoc_gridfs_file_t *file, const bson_t *bson);

Parameters
----------

* ``file``: A :symbol:`mongoc_gridfs_file_t`.
* ``bson``: A :symbol:`bson:bson_t` containing the aliases.

Description
-----------

Sets the aliases for a gridfs file.

You need to call :symbol:`mongoc_gridfs_file_save()` to persist this change.

