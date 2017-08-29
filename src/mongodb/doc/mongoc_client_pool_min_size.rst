:man_page: mongoc_client_pool_min_size

mongoc_client_pool_min_size()
=============================

Synopsis
--------

.. code-block:: c

  void
  mongoc_client_pool_min_size (mongoc_client_pool_t *pool,
                               uint32_t min_pool_size);

This function sets the minimum number of pooled connections kept in :symbol:`mongoc_client_pool_t`.

Parameters
----------

* ``pool``: A :symbol:`mongoc_client_pool_t`.
* ``min_pool_size``: The minimum number of connections which shall be kept in the pool.

.. include:: includes/mongoc_client_pool_thread_safe.txt

Subsequent calls to :symbol:`mongoc_client_pool_push` respect the new minimum size, and close the least recently used :symbol:`mongoc_client_t` if the minimum size is exceeded.
