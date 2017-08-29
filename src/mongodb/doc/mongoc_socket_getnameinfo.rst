:man_page: mongoc_socket_getnameinfo

mongoc_socket_getnameinfo()
===========================

Synopsis
--------

.. code-block:: c

  char *
  mongoc_socket_getnameinfo (mongoc_socket_t *sock);

Parameters
----------

* ``sock``: A :symbol:`mongoc_socket_t`.

Description
-----------

This is a helper around getting the local name of a socket. It is a wrapper around ``getpeername()`` and ``getnameinfo()``.

Returns
-------

A newly allocated string that should be freed with :symbol:`bson:bson_free()`.

