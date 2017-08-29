:man_page: mongoc_stream_write

mongoc_stream_write()
=====================

Synopsis
--------

.. code-block:: c

  ssize_t
  mongoc_stream_write (mongoc_stream_t *stream,
                       void *buf,
                       size_t count,
                       int32_t timeout_msec);

Parameters
----------

* ``stream``: A :symbol:`mongoc_stream_t`.
* ``buf``: The buffer to write.
* ``count``: The number of bytes to write.
* ``timeout_msec``: The number of milliseconds to wait before failure, a timeout of 0 will not block. If negative, use the default timeout.

The :symbol:`mongoc_stream_write()` function shall perform a write to a :symbol:`mongoc_stream_t`. It's modeled on the API and semantics of ``write()``, though the parameters map only loosely.

Returns
-------

The :symbol:`mongoc_stream_write` function returns the number of bytes write on success. It returns ``>= 0`` and ``< min_bytes`` when end-of-file is encountered and ``-1`` on failure. ``errno`` is set upon failure.

See Also
--------

:symbol:`mongoc_stream_read()`

:symbol:`mongoc_stream_readv()`

:symbol:`mongoc_stream_writev()`

