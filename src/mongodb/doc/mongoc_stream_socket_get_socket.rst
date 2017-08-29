:man_page: mongoc_stream_socket_get_socket

mongoc_stream_socket_get_socket()
=================================

Synopsis
--------

.. code-block:: c

  mongoc_socket_t *
  mongoc_stream_socket_get_socket (mongoc_stream_socket_t *stream);

Parameters
----------

* ``stream``: A :symbol:`mongoc_stream_socket_t`.

Retrieves the underlying :symbol:`mongoc_socket_t` for a :symbol:`mongoc_stream_socket_t`.

Returns
-------

A :symbol:`mongoc_stream_socket_t`.

