:man_page: mongoc_gridfs_file_set_id

mongoc_gridfs_file_set_id()
===========================

Synopsis
--------

.. code-block:: c

  bool
  mongoc_gridfs_file_set_id (mongoc_gridfs_file_t *file,
                             const bson_value_t *id,
                             bson_error_t error);

Parameters
----------

* ``file``: A :symbol:` mongoc_gridfs_file_t <mongoc_gridfs_file_t>`.
* ``id``: A :symbol:`bson:bson_value_t`.
* ``error``: A :symbol:`bson_error_t <errors>`.

Description
-----------

Sets the id of ``file`` to any BSON type.

If an error occurred, false is returned.

Returns
-------

Returns true on success. If any arguments are invalid, returns false and logs an error.
