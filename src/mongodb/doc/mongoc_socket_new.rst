:man_page: mongoc_socket_new

mongoc_socket_new()
===================

Synopsis
--------

.. code-block:: c

  mongoc_socket_t *
  mongoc_socket_new (int domain, int type, int protocol);

Parameters
----------

* ``domain``: An int containing the address family such as AF_INET.
* ``type``: An int containing the socket type such as SOCK_STREAM.
* ``protocol``: A protocol subset, typically 0.

Description
-----------

Creates a new ``mongoc_socket_t`` structure. This calls ``socket()`` underneath to create a network socket.

Returns
-------

A new socket if successful, otherwise ``NULL`` and errno is set. The result should be freed with :symbol:`mongoc_socket_destroy()` when no longer in use.

