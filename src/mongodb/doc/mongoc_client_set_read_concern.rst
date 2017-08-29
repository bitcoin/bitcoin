:man_page: mongoc_client_set_read_concern

mongoc_client_set_read_concern()
================================

Synopsis
--------

.. code-block:: c

  void
  mongoc_client_set_read_concern (mongoc_client_t *client,
                                  const mongoc_read_concern_t *read_concern);

Sets the read concern for the client. This only affects future operations, collections, and databases inheriting from ``client``.

The default read concern is MONGOC_READ_CONCERN_LEVEL_LOCAL. This is the correct read concern for the great majority of applications.

It is a programming error to call this function on a client from a :symbol:`mongoc_client_pool_t`. For pooled clients, set the read concern with the :ref:`MongoDB URI <mongoc_uri_t_read_concern_options>` instead.

Parameters
----------

* ``client``: A :symbol:`mongoc_client_t`.
* ``read_concern``: A :symbol:`mongoc_read_concern_t`.

