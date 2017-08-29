:man_page: mongoc_client_pool_t

mongoc_client_pool_t
====================

A connection pool for multi-threaded programs. See :doc:`connection-pooling`.

Synopsis
--------

.. code-block:: c

  typedef struct _mongoc_client_pool_t mongoc_client_pool_t

``mongoc_client_pool_t`` is the basis for multi-threading in the MongoDB C driver. Since :symbol:`mongoc_client_t` structures are not thread-safe, this structure is used to retrieve a new :symbol:`mongoc_client_t` for a given thread. This structure *is thread-safe*.

Example
-------

.. literalinclude:: ../examples/example-pool.c
   :language: c
   :caption: example-pool.c

.. only:: html

  Functions
  ---------

  .. toctree::
    :titlesonly:
    :maxdepth: 1

    mongoc_client_pool_destroy
    mongoc_client_pool_max_size
    mongoc_client_pool_min_size
    mongoc_client_pool_new
    mongoc_client_pool_pop
    mongoc_client_pool_push
    mongoc_client_pool_set_apm_callbacks
    mongoc_client_pool_set_appname
    mongoc_client_pool_set_error_api
    mongoc_client_pool_set_ssl_opts
    mongoc_client_pool_try_pop

