:man_page: mongoc_client_get_server_description

mongoc_client_get_server_description()
======================================

Synopsis
--------

.. code-block:: c

  mongoc_server_description_t *
  mongoc_client_get_server_description (mongoc_client_t *client,
                                        uint32_t server_id);

Get information about the server specified by ``server_id``.

Parameters
----------

* ``client``: A :symbol:`mongoc_client_t`.
* ``server_id``: An opaque id specifying the server.

Returns
-------

A :symbol:`mongoc_server_description_t` that must be freed with :symbol:`mongoc_server_description_destroy`. If the server is no longer in the topology, returns NULL.

