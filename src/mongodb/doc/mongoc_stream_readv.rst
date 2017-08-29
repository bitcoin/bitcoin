:man_page: mongoc_stream_readv

mongoc_stream_readv()
=====================

Synopsis
--------

.. code-block:: c

  ssize_t
  mongoc_stream_readv (mongoc_stream_t *stream,
                       mongoc_iovec_t *iov,
                       size_t iovcnt,
                       size_t min_bytes,
                       int32_t timeout_msec);

Parameters
----------

* ``stream``: A :symbol:`mongoc_stream_t`.
* ``iov``: A vector of :symbol:`mongoc_iovec_t`.
* ``iovcnt``: The number of items in ``iov``.
* ``min_bytes``: The minimum number of bytes to read or failure will be indicated.
* ``timeout_msec``: A timeout in milliseconds, or 0 to indicate non-blocking. A negative value with use the default timeout.

This function is identical to :symbol:`mongoc_stream_read()` except that it takes a :symbol:`mongoc_iovec_t` to perform gathered I/O.

Returns
-------

``>= 0`` on success, ``-1`` on failure and ``errno`` is set.

See Also
--------

:symbol:`mongoc_stream_read()`

:symbol:`mongoc_stream_write()`

:symbol:`mongoc_stream_writev()`

