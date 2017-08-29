:man_page: mongoc_gridfs_remove_by_filename

mongoc_gridfs_remove_by_filename()
==================================

Synopsis
--------

.. code-block:: c

  bool
  mongoc_gridfs_remove_by_filename (mongoc_gridfs_t *gridfs,
                                    const char *filename,
                                    bson_error_t *error);

Parameters
----------

* ``gridfs``: A :symbol:`mongoc_gridfs_t`.
* ``filename``: A UTF-8 encoded string containing the filename.
* ``error``: An optional location for a :symbol:`bson_error_t <errors>` or ``NULL``.

Description
-----------

Removes all files matching ``filename`` and their data chunks from the MongoDB server.

Returns
-------

Returns true if successful, including when no files match. Returns ``false`` and sets ``error`` if there are invalid arguments or a server or network error.

Errors
------

Errors are propagated via the ``error`` parameter.

