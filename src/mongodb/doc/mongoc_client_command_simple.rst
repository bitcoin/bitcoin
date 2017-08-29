:man_page: mongoc_client_command_simple

mongoc_client_command_simple()
==============================

Synopsis
--------

.. code-block:: c

  bool
  mongoc_client_command_simple (mongoc_client_t *client,
                                const char *db_name,
                                const bson_t *command,
                                const mongoc_read_prefs_t *read_prefs,
                                bson_t *reply,
                                bson_error_t *error);

This is a simplified interface to :symbol:`mongoc_client_command()`. It returns the first document from the result cursor into ``reply``. The client's read preference, read concern, and write concern are not applied to the command.

.. warning::

  ``reply`` is always set, and should be released with :symbol:`bson:bson_destroy()`.

Parameters
----------

* ``client``: A :symbol:`mongoc_client_t`.
* ``db_name``: The name of the database to run the command on.
* ``command``: A :symbol:`bson:bson_t` containing the command specification.
* ``read_prefs``: An optional :symbol:`mongoc_read_prefs_t`. Otherwise, the command uses mode ``MONGOC_READ_PRIMARY``.
* ``reply``: A location for the resulting document.
* ``error``: An optional location for a :symbol:`bson_error_t <errors>` or ``NULL``.

Errors
------

Errors are propagated via the ``error`` parameter.

Returns
-------

Returns ``true`` if successful. Returns ``false`` and sets ``error`` if there are invalid arguments or a server or network error.

This function does not check the server response for a write concern error or write concern timeout.

