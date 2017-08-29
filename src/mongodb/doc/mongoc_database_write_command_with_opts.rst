:man_page: mongoc_database_write_command_with_opts

mongoc_database_write_command_with_opts()
=========================================

Synopsis
--------

.. code-block:: c

  bool
  mongoc_database_write_command_with_opts (mongoc_database_t *database,
                                           const bson_t *command,
                                           const bson_t *opts,
                                           bson_t *reply,
                                           bson_error_t *error);

Execute a command on the server, applying logic that is specific to commands that write, and taking the MongoDB server version into account. To send a raw command to the server without any of this logic, use :symbol:`mongoc_database_command_simple`.

Use this function for commands that write such as "drop" or "createRole" (but not for "insert", "update", or "delete", see `Basic Write Operations`_). Write concern is applied from ``opts``, or else from ``database``. The write concern is omitted for MongoDB before 3.2. Collation is applied from ``opts`` (:ref:`see example for the "distinct" command with opts <mongoc_client_read_command_with_opts_example>`). Collation requires MongoDB 3.2 or later, otherwise an error is returned. No read concern or read preferences are applied.

To target a specific server, include an integer "serverId" field in ``opts`` with an id obtained first by calling :symbol:`mongoc_client_select_server`, then :symbol:`mongoc_server_description_id` on its return value.

``reply`` is always initialized, and must be freed with :symbol:`bson:bson_destroy()`.

Parameters
----------

* ``database``: A :symbol:`mongoc_database_t`.
* ``db_name``: The name of the database to run the command on.
* ``command``: A :symbol:`bson:bson_t` containing the command specification.
* ``opts``: A :symbol:`bson:bson_t` containing additional options.
* ``reply``: A location for the resulting document.
* ``error``: An optional location for a :symbol:`bson_error_t <errors>` or ``NULL``.

Errors
------

Errors are propagated via the ``error`` parameter.

Returns
-------

Returns ``true`` if successful. Returns ``false`` and sets ``error`` if there are invalid arguments or a server or network error.

A write concern timeout or write concern error is considered a failure.

Basic Write Operations
----------------------

Do not use this function to call the basic write commands "insert", "update", and "delete". Those commands require special logic not implemented in ``mongoc_database_write_command_with_opts``. For basic write operations use CRUD functions such as :symbol:`mongoc_collection_insert` and the others described in :ref:`the CRUD tutorial <tutorial_crud_operations>`, or use the :doc:`Bulk API <bulk>`.

Example
-------

See the example code for :symbol:`mongoc_client_read_command_with_opts`.

