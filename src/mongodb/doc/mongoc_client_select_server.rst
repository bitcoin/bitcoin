:man_page: mongoc_client_select_server

mongoc_client_select_server()
=============================

Synopsis
--------

.. code-block:: c

  mongoc_server_description_t *
  mongoc_client_select_server (mongoc_client_t *client,
                               bool for_writes,
                               const mongoc_read_prefs_t *prefs,
                               bson_error_t *error);

Choose a server for an operation, according to the logic described in the Server Selection Spec.

Use this function only for building a language driver that wraps the C Driver. When writing applications in C, higher-level functions automatically select a suitable server.

Parameters
----------

* ``client``: A :symbol:`mongoc_client_t`.
* ``for_writes``: Whether to choose a server suitable for writes or reads.
* ``prefs``: An optional :symbol:`mongoc_read_prefs_t`. If ``for_writes`` is true, ``prefs`` must be NULL. Otherwise, use ``prefs`` to customize server selection, or pass NULL to use the read preference configured on the client.
* ``error``: An optional location for a :symbol:`bson_error_t <errors>` or ``NULL``.

Returns
-------

A :symbol:`mongoc_server_description_t` that must be freed with :symbol:`mongoc_server_description_destroy`. If no suitable server is found, returns NULL and ``error`` is filled out.

