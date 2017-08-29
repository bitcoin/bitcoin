:man_page: mongoc_socket_bind

mongoc_socket_bind()
====================

Synopsis
--------

.. code-block:: c

  int
  mongoc_socket_bind (mongoc_socket_t *sock,
                      const struct sockaddr *addr,
                      mongoc_socklen_t addrlen);

Parameters
----------

* ``sock``: A :symbol:`mongoc_socket_t`.
* ``addr``: A struct sockaddr.
* ``addrlen``: A mongoc_socklen_t.

Description
-----------

This function is a wrapper around the BSD socket ``bind()`` interface. It provides better portability between UNIX-like and Microsoft Windows platforms.

Returns
-------

0 on success, -1 on failure and errno is set.

