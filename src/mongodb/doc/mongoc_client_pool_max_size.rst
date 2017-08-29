:man_page: mongoc_client_pool_max_size

mongoc_client_pool_max_size()
=============================

Synopsis
--------

.. code-block:: c

  void
  mongoc_client_pool_min_size (mongoc_client_pool_t *pool,
                               uint32_t max_pool_size);

This function sets the maximum number of pooled connections available from a :symbol:`mongoc_client_pool_t`.

Parameters
----------

* ``pool``: A :symbol:`mongoc_client_pool_t`.
* ``max_pool_size``: The maximum number of connections which shall be available from the pool.

.. include:: includes/mongoc_client_pool_thread_safe.txt
