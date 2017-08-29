:man_page: mongoc_gridfs_file_list_error

mongoc_gridfs_file_list_error()
===============================

Synopsis
--------

.. code-block:: c

  bool
  mongoc_gridfs_file_list_error (mongoc_gridfs_file_list_t *list,
                                 bson_error_t *error);

Parameters
----------

* ``list``: A :symbol:`mongoc_gridfs_file_list_t`.
* ``error``: An optional location for a :symbol:`bson_error_t <errors>` or ``NULL``.

Description
-----------

Fetches any error that has occurred while trying to retrieve the file list.

Errors
------

Errors are propagated via the ``error`` parameter.

Returns
-------

false if no error has been registered, otherwise true and error is set.

