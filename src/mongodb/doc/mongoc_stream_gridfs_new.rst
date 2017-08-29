:man_page: mongoc_stream_gridfs_new

mongoc_stream_gridfs_new()
==========================

Synopsis
--------

.. code-block:: c

  mongoc_stream_t *
  mongoc_stream_gridfs_new (mongoc_gridfs_file_t *file);

Parameters
----------

* ``file``: A :symbol:`mongoc_gridfs_file_t`.

This function shall create a new :symbol:`mongoc_gridfs_file_t`. This function does not transfer ownership of ``file``. Therefore, ``file`` must remain valid for the lifetime of this stream.

Returns
-------

A newly allocated :symbol:`mongoc_stream_gridfs_t` if successful, otherwise ``NULL``.

