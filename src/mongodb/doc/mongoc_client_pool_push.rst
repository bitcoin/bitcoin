:man_page: mongoc_client_pool_push

mongoc_client_pool_push()
=========================

Synopsis
--------

.. code-block:: c

  void
  mongoc_client_pool_push (mongoc_client_pool_t *pool, mongoc_client_t *client);

This function returns a :symbol:`mongoc_client_t` back to the client pool.

Parameters
----------

* ``pool``: A :symbol:`mongoc_client_pool_t`.
* ``client``: A :symbol:`mongoc_client_t`.

.. include:: includes/mongoc_client_pool_thread_safe.txt
