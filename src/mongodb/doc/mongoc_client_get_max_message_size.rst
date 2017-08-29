:man_page: mongoc_client_get_max_message_size

mongoc_client_get_max_message_size()
====================================

Synopsis
--------

.. code-block:: c

  int32_t
  mongoc_client_get_max_message_size (mongoc_client_t *client);

The :symbol:`mongoc_client_get_max_message_size()` returns the maximum message size allowed by the cluster. Until a connection has been made, this will be the default of 40Mb.

Deprecated
----------

.. warning::

  This function is deprecated and should not be used in new code.

Parameters
----------

* ``client``: A :symbol:`mongoc_client_t`.

Returns
-------

The server provided max message size, or 40Mb if no connection has been established.

