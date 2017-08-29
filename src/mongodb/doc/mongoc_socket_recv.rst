:man_page: mongoc_socket_recv

mongoc_socket_recv()
====================

Synopsis
--------

.. code-block:: c

  ssize_t
  mongoc_socket_recv (mongoc_socket_t *sock,
                      void *buf,
                      size_t buflen,
                      int flags,
                      int64_t expire_at);

Parameters
----------

* ``sock``: A :symbol:`mongoc_socket_t`.
* ``buf``: A buffer to read into.
* ``buflen``: A size_t with the number of bytes to receive.
* ``flags``: flags for ``recv()``.
* ``expire_at``: A int64_t with the time to expire in monotonic time using :symbol:`bson:bson_get_monotonic_time()`, which is in microseconds.

Description
-----------

This function performs a ``recv()`` on the underlying socket.

Returns
-------

The number of bytes received on success, 0 on stream closed, and -1 if there was a failure and errno is set.

