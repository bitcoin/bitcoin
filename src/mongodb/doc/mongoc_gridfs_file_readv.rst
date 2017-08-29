:man_page: mongoc_gridfs_file_readv

mongoc_gridfs_file_readv()
==========================

Synopsis
--------

.. code-block:: c

  ssize_t
  mongoc_gridfs_file_readv (mongoc_gridfs_file_t *file,
                            mongoc_iovec_t *iov,
                            size_t iovcnt,
                            size_t min_bytes,
                            uint32_t timeout_msec);

Parameters
----------

* ``file``: A :symbol:`mongoc_gridfs_file_t`.
* ``iov``: An array of :symbol:`mongoc_iovec_t`.
* ``iovcnt``: The number of elements in ``iov``.
* ``min_bytes``: The minimum number of bytes that must be read or an error will be synthesized.
* ``timeout_msec``: Unused.

Description
-----------

This function performs a scattered read from ``file``, potentially blocking to read from the MongoDB server.

The ``timeout_msec`` parameter is unused.

Returns
-------

Returns the number of bytes read, or -1 on failure. Use :symbol:`mongoc_gridfs_file_error` to retrieve error details.

