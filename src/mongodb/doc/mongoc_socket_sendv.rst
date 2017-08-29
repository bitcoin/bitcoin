:man_page: mongoc_socket_sendv

mongoc_socket_sendv()
=====================

Synopsis
--------

.. code-block:: c

  ssize_t
  mongoc_socket_sendv (mongoc_socket_t *sock,
                       mongoc_iovec_t *iov,
                       size_t iovcnt,
                       int64_t expire_at);

Parameters
----------

* ``sock``: A :symbol:`mongoc_socket_t`.
* ``iov``: A mongoc_iovec_t.
* ``iovcnt``: A size_t containing the number of elements in iov.
* ``expire_at``: A int64_t with absolute timeout in monotonic time. The monotonic clock is in microseconds and can be fetched using :symbol:`bson:bson_get_monotonic_time()`.

Description
-----------

Sends a vector of buffers to the destination. This uses ``sendmsg()`` when available to perform a gathered write. If IOV_MAX is reached, a fallback will be used.

Returns
-------

the number of bytes sent on success, or -1 on failure and errno is set.

