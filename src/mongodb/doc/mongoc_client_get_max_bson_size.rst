:man_page: mongoc_client_get_max_bson_size

mongoc_client_get_max_bson_size()
=================================

Synopsis
--------

.. code-block:: c

  int32_t
  mongoc_client_get_max_bson_size (mongoc_client_t *client);

The :symbol:`mongoc_client_get_max_bson_size()` returns the maximum bson document size allowed by the cluster. Until a connection has been made, this will be the default of 16Mb.

Deprecated
----------

.. warning::

  This function is deprecated and should not be used in new code.

Parameters
----------

* ``client``: A :symbol:`mongoc_client_t`.

Returns
-------

The server provided max bson size, or 16Mb if no connection has been established.

