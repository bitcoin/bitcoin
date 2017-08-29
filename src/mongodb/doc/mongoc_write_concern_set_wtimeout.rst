:man_page: mongoc_write_concern_set_wtimeout

mongoc_write_concern_set_wtimeout()
===================================

Synopsis
--------

.. code-block:: c

  void
  mongoc_write_concern_set_wtimeout (mongoc_write_concern_t *write_concern,
                                     int32_t wtimeout_msec);

Parameters
----------

* ``write_concern``: A :symbol:`mongoc_write_concern_t`.
* ``wtimeout_msec``: A positive ``int32_t`` or zero.

Description
-----------

Set the timeout in milliseconds that the server should wait before returning with a write concern timeout. This is not the same as a socket timeout. A value of zero may be used to indicate no write concern timeout.

