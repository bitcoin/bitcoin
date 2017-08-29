:man_page: mongoc_client_pool_set_appname

mongoc_client_pool_set_appname()
================================

Synopsis
--------

.. code-block:: c

  bool
  mongoc_client_pool_set_appname (mongoc_client_pool_t *pool, const char *appname)

This function is identical to :symbol:`mongoc_client_set_appname()` except for client pools.

Also note that :symbol:`mongoc_client_set_appname()` cannot be called on a client retrieved from a client pool.

Parameters
----------

* ``pool``: A :symbol:`mongoc_client_pool_t`.
* ``appname``: The application name, of length at most ``MONGOC_HANDSHAKE_APPNAME_MAX``.

Returns
-------

Returns true if appname was set. If appname is too long, returns false and logs an error.

.. include:: includes/mongoc_client_pool_call_once.txt
