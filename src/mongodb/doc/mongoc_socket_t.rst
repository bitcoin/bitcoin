:man_page: mongoc_socket_t

mongoc_socket_t
===============

Portable socket abstraction

Synopsis
--------

.. code-block:: c

  #include <mongoc.h>

  typedef struct _mongoc_socket_t mongoc_socket_t

Synopsis
--------

This structure provides a socket abstraction that is friendlier for portability than BSD sockets directly. Inconsistencies between Linux, various BSDs, Solaris, and Windows are handled here.

.. only:: html

  Functions
  ---------

  .. toctree::
    :titlesonly:
    :maxdepth: 1

    mongoc_socket_accept
    mongoc_socket_bind
    mongoc_socket_close
    mongoc_socket_connect
    mongoc_socket_destroy
    mongoc_socket_errno
    mongoc_socket_getnameinfo
    mongoc_socket_getsockname
    mongoc_socket_listen
    mongoc_socket_new
    mongoc_socket_recv
    mongoc_socket_send
    mongoc_socket_sendv
    mongoc_socket_setsockopt

