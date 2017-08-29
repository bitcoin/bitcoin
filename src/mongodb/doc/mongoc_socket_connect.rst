:man_page: mongoc_socket_connect

mongoc_socket_connect()
=======================

Synopsis
--------

.. code-block:: c

  int
  mongoc_socket_connect (mongoc_socket_t *sock,
                         const struct sockaddr *addr,
                         mongoc_socklen_t addrlen,
                         int64_t expire_at);

Parameters
----------

* ``sock``: A :symbol:`mongoc_socket_t`.
* ``addr``: A struct sockaddr.
* ``addrlen``: A mongoc_socklen_t.
* ``expire_at``: A int64_t containing the absolute timeout using the monotonic clock.

Description
-----------

This function is a wrapper around the BSD socket ``connect()`` interface. It provides better portability between UNIX-like and Microsoft Windows platforms.

This function performs a socket connection but will fail if ``expire_at`` has been reached by the monotonic clock. Keep in mind that this is an absolute timeout in milliseconds. You should add your desired timeout to :symbol:`bson:bson_get_monotonic_time()`.

Returns
-------

0 if successful, -1 on failure and errno is set.

