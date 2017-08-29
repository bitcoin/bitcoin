:man_page: mongoc_socket_getsockname

mongoc_socket_getsockname()
===========================

Synopsis
--------

.. code-block:: c

  int
  mongoc_socket_getsockname (mongoc_socket_t *sock,
                             struct sockaddr *addr,
                             mongoc_socklen_t *addrlen);

Parameters
----------

* ``sock``: A :symbol:`mongoc_socket_t`.
* ``addr``: A struct sockaddr.
* ``addrlen``: A mongoc_socklen_t.

Description
-----------

Retrieves the socket name for ``sock``. The result is stored in ``addr``. ``addrlen`` should be the size of the ``addr`` when calling this.

Returns
-------

0 if successful, otherwise -1 and errno is set.

