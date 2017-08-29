:man_page: mongoc_client_command_simple_with_server_id

mongoc_client_command_simple_with_server_id()
=============================================

Synopsis
--------

.. code-block:: c

  bool
  mongoc_client_command_simple_with_server_id (
     mongoc_client_t *client,
     const char *db_name,
     const bson_t *command,
     const mongoc_read_prefs_t *read_prefs,
     uint32_t server_id bson_t *reply,
     bson_error_t *error);

This function executes a command on a specific server, using the database and command specification provided.

Parameters
----------

* ``client``: A :symbol:`mongoc_client_t`.
* ``db_name``: The name of the database to run the command on.
* ``read_prefs``: An optional :symbol:`mongoc_read_prefs_t`.
* ``server_id``: An opaque id specifying which server to use.
* ``reply``: An optional location for a :symbol:`bson:bson_t` which will store the server's reply.
* ``error``: An optional location for a :symbol:`bson_error_t <errors>` or a ``NULL``.

Returns
-------

Returns ``true`` if successful. Returns ``false`` and sets ``error`` if there are invalid arguments or a server or network error.

This function does not check the server response for a write concern error or write concern timeout.

