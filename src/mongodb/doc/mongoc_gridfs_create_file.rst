:man_page: mongoc_gridfs_create_file

mongoc_gridfs_create_file()
===========================

Synopsis
--------

.. code-block:: c

  mongoc_gridfs_file_t *
  mongoc_gridfs_create_file (mongoc_gridfs_t *gridfs,
                             mongoc_gridfs_file_opt_t *opt);

Parameters
----------

* ``gridfs``: A :symbol:`mongoc_gridfs_t`.
* ``opt``: A :symbol:`mongoc_gridfs_file_opt_t` to specify file options.

Description
-----------

This function shall create a new :symbol:`mongoc_gridfs_file_t`.

Use :symbol:`mongoc_gridfs_file_writev()` to write to the file.

Returns
-------

Returns a newly allocated :symbol:`mongoc_gridfs_file_t` that should be freed with :symbol:`mongoc_gridfs_file_destroy()`.

