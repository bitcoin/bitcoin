:man_page: mongoc_write_concern_get_wtimeout

mongoc_write_concern_get_wtimeout()
===================================

Synopsis
--------

.. code-block:: c

  int32_t
  mongoc_write_concern_get_wtimeout (const mongoc_write_concern_t *write_concern);

Parameters
----------

* ``write_concern``: A :symbol:`mongoc_write_concern_t`.

Description
-----------

Get the timeout in milliseconds that the server should wait before returning with a write concern timeout.

A value of 0 indicates no write timeout.

Returns
-------

Returns an 32-bit signed integer containing the timeout.

