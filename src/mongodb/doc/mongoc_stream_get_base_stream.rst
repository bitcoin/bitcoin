:man_page: mongoc_stream_get_base_stream

mongoc_stream_get_base_stream()
===============================

Synopsis
--------

.. code-block:: c

  mongoc_stream_t *
  mongoc_stream_get_base_stream (mongoc_stream_t *stream);

Parameters
----------

* ``stream``: A :symbol:`mongoc_stream_t`.

This function shall fetch the underlying stream for streams that wrap a base stream. Such implementations include :symbol:`mongoc_stream_buffered_t` and :symbol:`mongoc_stream_tls_t`.

Returns
-------

A :symbol:`mongoc_stream_t` or ``NULL``.

