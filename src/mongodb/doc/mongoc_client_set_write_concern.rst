:man_page: mongoc_client_set_write_concern

mongoc_client_set_write_concern()
=================================

Synopsis
--------

.. code-block:: c

  void
  mongoc_client_set_write_concern (mongoc_client_t *client,
                                   const mongoc_write_concern_t *write_concern);

Sets the write concern for the client. This only affects future operations, collections, and databases inheriting from ``client``.

The default write concern is MONGOC_WRITE_CONCERN_W_DEFAULT: the driver blocks awaiting basic acknowledgment of write operations from MongoDB. This is the correct write concern for the great majority of applications.

It is a programming error to call this function on a client from a :symbol:`mongoc_client_pool_t`. For pooled clients, set the write concern with the :ref:`MongoDB URI <mongoc_uri_t_write_concern_options>` instead.

Parameters
----------

* ``client``: A :symbol:`mongoc_client_t`.
* ``write_concern``: A :symbol:`mongoc_write_concern_t`.

