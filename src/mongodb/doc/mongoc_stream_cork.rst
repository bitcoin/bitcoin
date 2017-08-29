:man_page: mongoc_stream_cork

mongoc_stream_cork()
====================

Synopsis
--------

.. code-block:: c

  int
  mongoc_stream_cork (mongoc_stream_t *stream);

Parameters
----------

* ``stream``: A :symbol:`mongoc_stream_t`.

This function shall prevent the writing of bytes to the underlying socket.

.. note::

  Not all streams implement this function. Buffering generally works better.

Returns
-------

``0`` on success, ``-1`` on failure and ``errno`` is set.

See Also
--------

:doc:`mongoc_stream_buffered_new() <mongoc_stream_buffered_new>`.

:doc:`mongoc_stream_uncork() <mongoc_stream_uncork>`.

