:man_page: mongoc_socket_close

mongoc_socket_close()
=====================

Synopsis
--------

.. code-block:: c

  int
  mongoc_socket_close (mongoc_socket_t *socket);

Parameters
----------

* ``socket``: A :symbol:`mongoc_socket_t`.

Description
-----------

This function is a wrapper around the BSD socket ``shutdown()`` and ``close()`` functions, and their Windows equivalents. The socket is shut down only if the current process is the same as the process that opened the socket. Regardless, the socket is then closed.

Returns
-------

0 on success, -1 on failure to close the socket. On failure, the socket's errno is set; retrieve it with :symbol:`mongoc_socket_errno()`.

