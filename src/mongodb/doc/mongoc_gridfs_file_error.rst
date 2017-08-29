:man_page: mongoc_gridfs_file_error

mongoc_gridfs_file_error()
==========================

Synopsis
--------

.. code-block:: c

  bool
  mongoc_gridfs_file_error (mongoc_gridfs_file_t *file, bson_error_t *error);

Parameters
----------

* ``file``: A :symbol:`mongoc_gridfs_file_t`.
* ``error``: An optional location for a :symbol:`bson_error_t <errors>` or ``NULL``.

Description
-----------

This function checks to see if there has been an error associated with the last operation upon ``file``.

Errors
------

Errors are propagated via the ``error`` parameter.

Returns
-------

Returns false if there has been no registered error, otherwise true and error is set.

