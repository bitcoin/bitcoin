:man_page: mongoc_stream_setsockopt

mongoc_stream_setsockopt()
==========================

Synopsis
--------

.. code-block:: c

  int
  mongoc_stream_setsockopt (mongoc_stream_t *stream,
                            int level,
                            int optname,
                            void *optval,
                            mongoc_socklen_t optlen);

Parameters
----------

* ``stream``: A :symbol:`mongoc_stream_t`.
* ``level``: The level to pass to ``setsockopt()``.
* ``optname``: The optname to pass to ``setsockopt()``.
* ``optval``: The optval to pass to ``setsockopt()``.
* ``optlen``: The optlen to pass to ``setsockopt()``.

This function is a wrapper around ``setsockopt()`` for streams that wrap sockets.

Returns
-------

``0`` on success, otherwise ``-1`` and ``errno`` is set.

