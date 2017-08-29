:man_page: mongoc_database_command_simple

mongoc_database_command_simple()
================================

Synopsis
--------

.. code-block:: c

  bool
  mongoc_database_command_simple (mongoc_database_t *database,
                                  const bson_t *command,
                                  const mongoc_read_prefs_t *read_prefs,
                                  bson_t *reply,
                                  bson_error_t *error);

Parameters
----------

* ``database``: A :symbol:`mongoc_database_t`.
* ``command``: A :symbol:`bson:bson_t` containing the command.
* ``read_prefs``: An optional :symbol:`mongoc_read_prefs_t`. Otherwise, the command uses mode ``MONGOC_READ_PRIMARY``.
* ``reply``: A location to store the commands first result document.
* ``error``: An optional location for a :symbol:`bson_error_t <errors>` or ``NULL``.

This is a simplified interface to :symbol:`mongoc_database_command()` that returns the first result document. The database's read preference, read concern, and write concern are not applied to the command.  The parameter ``reply`` is initialized even upon failure to simplify memory management.

Errors
------

Errors are propagated through the ``error`` parameter.

Returns
-------

Returns ``true`` if successful. Returns ``false`` and sets ``error`` if there are invalid arguments or a server or network error.

This function does not check the server response for a write concern error or write concern timeout.

