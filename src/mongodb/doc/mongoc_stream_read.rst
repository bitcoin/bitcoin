:man_page: mongoc_stream_read

mongoc_stream_read()
====================

Synopsis
--------

.. code-block:: c

  ssize_t
  mongoc_stream_read (mongoc_stream_t *stream,
                      void *buf,
                      size_t count,
                      size_t min_bytes,
                      int32_t timeout_msec);

Parameters
----------

* ``stream``: A :symbol:`mongoc_stream_t`.
* ``buf``: The buffer to read into.
* ``count``: The number of bytes to read.
* ``min_bytes``: The minimum number of bytes to read, or else indicate failure.
* ``timeout_msec``: The number of milliseconds to wait before failure, a timeout of 0 will not block. If negative, use the default timeout.

The :symbol:`mongoc_stream_read()` function shall perform a read from a :symbol:`mongoc_stream_t`. It's modeled on the API and semantics of ``read()``, though the parameters map only loosely.

Returns
-------

The :symbol:`mongoc_stream_read` function returns the number of bytes read on success. It returns ``>= 0`` and ``< min_bytes`` when end-of-file is encountered and ``-1`` on failure. ``errno`` is set upon failure.

See Also
--------

:symbol:`mongoc_stream_readv()`

:symbol:`mongoc_stream_write()`

:symbol:`mongoc_stream_writev()`

