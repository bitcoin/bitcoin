:man_page: mongoc_client_get_write_concern

mongoc_client_get_write_concern()
=================================

Synopsis
--------

.. code-block:: c

  const mongoc_write_concern_t *
  mongoc_client_get_write_concern (const mongoc_client_t *client);

Retrieve the default write concern configured for the client instance. The result should not be modified or freed.

Parameters
----------

* ``client``: A :symbol:`mongoc_client_t`.

Returns
-------

A :symbol:`mongoc_write_concern_t`.

