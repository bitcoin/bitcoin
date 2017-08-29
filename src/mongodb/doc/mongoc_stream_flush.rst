:man_page: mongoc_stream_flush

mongoc_stream_flush()
=====================

Synopsis
--------

.. code-block:: c

  int
  mongoc_stream_flush (mongoc_stream_t *stream);

Parameters
----------

* ``stream``: A :symbol:`mongoc_stream_t`.

This function shall flush any buffered bytes in the underlying stream to the physical transport. It mimics the API and semantics of ``fflush()``, forcing a write of user space buffered data.

Not all stream implementations may implement this feature.

Returns
-------

0 is returned on success, otherwise ``-1`` and ``errno`` is set.

