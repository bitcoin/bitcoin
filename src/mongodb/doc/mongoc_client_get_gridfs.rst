:man_page: mongoc_client_get_gridfs

mongoc_client_get_gridfs()
==========================

Synopsis
--------

.. code-block:: c

  mongoc_gridfs_t *
  mongoc_client_get_gridfs (mongoc_client_t *client,
                            const char *db,
                            const char *prefix,
                            bson_error_t *error);

The ``mongoc_client_get_gridfs()`` function shall create a new :symbol:`mongoc_gridfs_t`. The ``db`` parameter is the name of the database which the gridfs instance should exist in. The ``prefix`` parameter corresponds to the gridfs collection namespacing; its default is "fs", thus the default GridFS collection names are "fs.files" and "fs.chunks".

Parameters
----------

* ``client``: A :symbol:`mongoc_client_t`.
* ``db``: The database name.
* ``prefix``: Optional prefix for gridfs collection names or ``NULL``.
* ``error``: An optional location for a :symbol:`bson_error_t <errors>` or ``NULL``.

Errors
------

Errors are propagated via the ``error`` parameter.

Returns
-------

A :symbol:`mongoc_gridfs_t` on success, ``NULL`` upon failure and ``error`` is set.

