:man_page: mongoc_gridfs_file_writev

mongoc_gridfs_file_writev()
===========================

Synopsis
--------

.. code-block:: c

  ssize_t
  mongoc_gridfs_file_writev (mongoc_gridfs_file_t *file,
                             const mongoc_iovec_t *iov,
                             size_t iovcnt,
                             uint32_t timeout_msec);

Parameters
----------

* ``file``: A :symbol:`mongoc_gridfs_file_t`.
* ``iov``: An array of :symbol:`mongoc_iovec_t`.
* ``iovcnt``: The number of elements in ``iov``.
* ``timeout_msec``: Unused.

Description
-----------

Performs a gathered write to the underlying gridfs file.

The ``timeout_msec`` parameter is unused.

Returns
-------

Returns the number of bytes written, or -1 on failure. Use :symbol:`mongoc_gridfs_file_error` to retrieve error details.
