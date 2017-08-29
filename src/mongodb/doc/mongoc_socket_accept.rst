:man_page: mongoc_socket_accept

mongoc_socket_accept()
======================

Synopsis
--------

.. code-block:: c

  mongoc_socket_t *
  mongoc_socket_accept (mongoc_socket_t *sock, int64_t expire_at);

Parameters
----------

* ``sock``: A :symbol:`mongoc_socket_t`.
* ``expire_at``: An int64_t containing a timeout in milliseconds.

Description
-----------

This function is a wrapper around the BSD socket ``accept()`` interface. It allows for more portability between UNIX-like and Microsoft Windows platforms.

Returns
-------

NULL upon failure to accept or timeout. A newly allocated ``mongoc_socket_t`` that should be released with :symbol:`mongoc_socket_destroy()` on success.

