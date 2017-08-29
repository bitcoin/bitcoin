:man_page: mongoc_socket_send

mongoc_socket_send()
====================

Synopsis
--------

.. code-block:: c

  ssize_t
  mongoc_socket_send (mongoc_socket_t *sock,
                      const void *buf,
                      size_t buflen,
                      int64_t expire_at);

Parameters
----------

* ``sock``: A :symbol:`mongoc_socket_t`.
* ``buf``: A buffer to send.
* ``buflen``: A size_t with the number of bytes in buf.
* ``expire_at``: A int64_t with an absolute timeout for the operation or 0. The timeout is in monotonic time using microseconds. You can retrieve the current monotonic time with :symbol:`bson:bson_get_monotonic_time()`.

Description
-----------

Sends buflen bytes in buf to the destination. If a timeout expired, the number of bytes sent will be returned or -1 if no bytes were sent.

Returns
-------

-1 on failure and errno is set, or the number of bytes sent.

