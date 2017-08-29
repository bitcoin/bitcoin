:man_page: mongoc_socket_setsockopt

mongoc_socket_setsockopt()
==========================

Synopsis
--------

.. code-block:: c

  int
  mongoc_socket_setsockopt (mongoc_socket_t *sock,
                            int level,
                            int optname,
                            const void *optval,
                            mongoc_socklen_t optlen);

Parameters
----------

* ``sock``: A :symbol:`mongoc_socket_t`.
* ``level``: A sockopt level.
* ``optname``: A sockopt name.
* ``optval``: A the value for the sockopt.
* ``optlen``: A mongoc_socklen_t that contains the length of optval.

Description
-----------

This is a helper function for ``setsockopt()``.

Returns
-------

0 on success, -1 on failure and errno is set.

