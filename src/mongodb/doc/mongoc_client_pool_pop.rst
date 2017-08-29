:man_page: mongoc_client_pool_pop

mongoc_client_pool_pop()
========================

Synopsis
--------

.. code-block:: c

  mongoc_client_t *
  mongoc_client_pool_pop (mongoc_client_pool_t *pool);

Retrieve a :symbol:`mongoc_client_t` from the client pool, possibly blocking until one is available.

Parameters
----------

* ``pool``: A :symbol:`mongoc_client_pool_t`.

Returns
-------

A :symbol:`mongoc_client_t`.

.. include:: includes/mongoc_client_pool_thread_safe.txt
