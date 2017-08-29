:man_page: mongoc_socket_errno

mongoc_socket_errno()
=====================

Synopsis
--------

.. code-block:: c

  int
  mongoc_socket_errno (mongoc_socket_t *sock);

Parameters
----------

* ``sock``: A :symbol:`mongoc_socket_t`.

Description
-----------

This function returns the currently captured ``errno`` for a socket. This may be useful to check was the last errno was after another function call has been made that clears the threads errno variable.

Returns
-------

0 if there is no error, otherwise a specific errno.

