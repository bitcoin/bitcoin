:man_page: mongoc_socket_listen

mongoc_socket_listen()
======================

Synopsis
--------

.. code-block:: c

  int
  mongoc_socket_listen (mongoc_socket_t *sock, unsigned int backlog);

Parameters
----------

* ``sock``: A :symbol:`mongoc_socket_t`.
* ``backlog``: An int containing max backlog size.

Description
-----------

This function is similar to the BSD sockets ``listen()`` function. It is meant for socket servers.

Returns
-------

0 on success, -1 on failure and errno is set.

