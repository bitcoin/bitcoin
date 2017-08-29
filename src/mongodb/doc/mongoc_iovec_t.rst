:man_page: mongoc_iovec_t

mongoc_iovec_t
==============

Synopsis
--------

Synopsis
--------

.. code-block:: c

  #include <mongoc.h>

  #ifdef _WIN32
  typedef struct {
     u_long iov_len;
     char *iov_base;
  } mongoc_iovec_t;
  #else
  typedef struct iovec mongoc_iovec_t;
  #endif

The ``mongoc_iovec_t`` structure is a portability abstraction for consumers of the :symbol:`mongoc_stream_t` interfaces. It allows for scatter/gather I/O through the socket subsystem.

.. warning::

  When writing portable code, beware of the ordering of ``iov_len`` and ``iov_base`` as they are different on various platforms. Therefore, you should not use C initializers for initialization.

