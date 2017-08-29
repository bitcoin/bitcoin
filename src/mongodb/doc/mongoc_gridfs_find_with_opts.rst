:man_page: mongoc_gridfs_find_with_opts

mongoc_gridfs_find_with_opts()
==============================

Synopsis
--------

.. code-block:: c

  mongoc_gridfs_file_list_t *
  mongoc_gridfs_find_with_opts (mongoc_gridfs_t *gridfs,
                                const bson_t *filter,
                                const bson_t *opts) BSON_GNUC_WARN_UNUSED_RESULT;

Parameters
----------

* ``gridfs``: A :symbol:`mongoc_gridfs_t`.
* ``filter``: A :symbol:`bson:bson_t` containing the query to execute.
* ``opts``: A :symbol:`bson:bson_t` query options, including sort order and which fields to return. Can be ``NULL``.

Description
-----------

Finds all gridfs files matching ``filter``. You can iterate the matched gridfs files with the resulting file list.

See :symbol:`mongoc_collection_find_with_opts` for a description of the ``filter`` and ``opts`` parameters.

Returns
-------

A newly allocated :symbol:`mongoc_gridfs_file_list_t` that should be freed with :symbol:`mongoc_gridfs_file_list_destroy()` when no longer in use.

