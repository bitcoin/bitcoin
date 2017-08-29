:man_page: mongoc_client_read_command_with_opts

mongoc_client_read_command_with_opts()
======================================

Synopsis
--------

.. code-block:: c

  bool
  mongoc_client_read_command_with_opts (mongoc_client_t *client,
                                        const char *db_name,
                                        const bson_t *command,
                                        const mongoc_read_prefs_t *read_prefs,
                                        const bson_t *opts,
                                        bson_t *reply,
                                        bson_error_t *error);

Execute a command on the server, applying logic that is specific to commands that read, and taking the MongoDB server version into account. To send a raw command to the server without any of this logic, use :symbol:`mongoc_client_command_simple`.

Use this function for commands that read such as "count" or "distinct". Read concern is applied from ``opts`` or else from ``client``. Collation is applied from ``opts`` (see example below). Read concern and collation both require MongoDB 3.2 or later, otherwise an error is returned. Read preferences are applied from ``read_prefs`` or else from ``client``. No write concern is applied.

To target a specific server, include an integer "serverId" field in ``opts`` with an id obtained first by calling :symbol:`mongoc_client_select_server`, then :symbol:`mongoc_server_description_id` on its return value.

``reply`` is always initialized, and must be freed with :symbol:`bson:bson_destroy()`.

Parameters
----------

* ``client``: A :symbol:`mongoc_client_t`.
* ``db_name``: The name of the database to run the command on.
* ``command``: A :symbol:`bson:bson_t` containing the command specification.
* ``read_prefs``: An optional :symbol:`mongoc_read_prefs_t`.
* ``opts``: A :symbol:`bson:bson_t` containing additional options.
* ``reply``: A location for the resulting document.
* ``error``: An optional location for a :symbol:`bson_error_t <errors>` or ``NULL``.

Errors
------

Errors are propagated via the ``error`` parameter.

Returns
-------

Returns ``true`` if successful. Returns ``false`` and sets ``error`` if there are invalid arguments or a server or network error.

.. _mongoc_client_read_command_with_opts_example:

Example
-------

.. literalinclude:: ../examples/example-command-with-opts.c
   :language: c
   :caption: example-command-with-opts.c

