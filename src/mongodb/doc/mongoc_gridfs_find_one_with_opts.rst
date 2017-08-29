:man_page: mongoc_gridfs_find_one_with_opts

mongoc_gridfs_find_one_with_opts()
==================================

Synopsis
--------

.. code-block:: c

  mongoc_gridfs_file_t *
  mongoc_gridfs_find_one_with_opts (mongoc_gridfs_t *gridfs,
                                    const bson_t *filter,
                                    const bson_t *opts,
                                    bson_error_t *error)
     BSON_GNUC_WARN_UNUSED_RESULT;

Parameters
----------

* ``gridfs``: A :symbol:`mongoc_gridfs_t`.
* ``filter``: A :symbol:`bson:bson_t` containing the query to execute.
* ``opts``: A :symbol:`bson:bson_t` query options, including sort order and which fields to return. Can be ``NULL``.
* ``error``: An optional location for a :symbol:`bson_error_t <errors>` or ``NULL``.

Description
-----------

Find the first GridFS file matching ``filter``.

See :symbol:`mongoc_collection_find_with_opts` for a description of the ``filter`` and ``opts`` parameters.

Errors
------

Errors are propagated via the ``error`` parameter.

Returns
-------

A newly allocated :symbol:`mongoc_gridfs_file_t` or ``NULL`` if no file could be found. You must free the resulting file with :symbol:`mongoc_gridfs_file_destroy()` if non-NULL.

