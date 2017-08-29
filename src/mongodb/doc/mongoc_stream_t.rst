:man_page: mongoc_stream_t

mongoc_stream_t
===============

Synopsis
--------

.. code-block:: c

  typedef struct _mongoc_stream_t mongoc_stream_t

``mongoc_stream_t`` provides a generic streaming IO abstraction based on a struct of pointers interface. The idea is to allow wrappers, perhaps other language drivers, to easily shim their IO system on top of ``mongoc_stream_t``.

The API for the stream abstraction is currently private and non-extensible.

Stream Types
------------

There are a number of built in stream types that come with mongoc. The default configuration is a buffered unix stream.  If SSL is in use, that in turn is wrapped in a tls stream.

.. only:: html

  Functions
  ---------

  .. toctree::
    :titlesonly:
    :maxdepth: 1

    mongoc_stream_buffered_new
    mongoc_stream_close
    mongoc_stream_cork
    mongoc_stream_destroy
    mongoc_stream_flush
    mongoc_stream_get_base_stream
    mongoc_stream_read
    mongoc_stream_readv
    mongoc_stream_setsockopt
    mongoc_stream_uncork
    mongoc_stream_write
    mongoc_stream_writev

See Also
--------

:doc:`mongoc_stream_buffered_t`

:doc:`mongoc_stream_file_t`

:doc:`mongoc_stream_socket_t`

:doc:`mongoc_stream_tls_t`

:doc:`mongoc_stream_gridfs_t`

