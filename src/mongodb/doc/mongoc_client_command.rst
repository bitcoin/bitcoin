:man_page: mongoc_client_command

mongoc_client_command()
=======================

Synopsis
--------

.. code-block:: c

  mongoc_cursor_t *
  mongoc_client_command (mongoc_client_t *client,
                         const char *db_name,
                         mongoc_query_flags_t flags,
                         uint32_t skip,
                         uint32_t limit,
                         uint32_t batch_size,
                         const bson_t *query,
                         const bson_t *fields,
                         const mongoc_read_prefs_t *read_prefs);

Superseded by :symbol:`mongoc_client_read_command_with_opts()`, :symbol:`mongoc_client_write_command_with_opts()`, and :symbol:`mongoc_client_read_write_command_with_opts()`.

This function creates a cursor which will execute the command when :symbol:`mongoc_cursor_next` is called on it. The client's read preference, read concern, and write concern are not applied to the command, and :symbol:`mongoc_cursor_next` will not check the server response for a write concern error or write concern timeout.

If :symbol:`mongoc_cursor_next()` returns ``false``, then retrieve error details with :symbol:`mongoc_cursor_error()` or :symbol:`mongoc_cursor_error_document()`.

Parameters
----------

* ``client``: A :symbol:`mongoc_client_t`.
* ``db_name``: The name of the database to run the command on.
* ``flags``: Unused.
* ``skip``: Unused.
* ``limit``: Unused.
* ``batch_size``: Unused.
* ``query``: A :symbol:`bson:bson_t` containing the command specification.
* ``fields``: Unused.
* ``read_prefs``: An optional :symbol:`mongoc_read_prefs_t`. Otherwise, the command uses mode ``MONGOC_READ_PRIMARY``.

Returns
-------

A :symbol:`mongoc_cursor_t`.

The cursor should be freed with :symbol:`mongoc_cursor_destroy()`.

