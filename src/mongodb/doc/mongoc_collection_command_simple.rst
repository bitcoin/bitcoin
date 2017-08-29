:man_page: mongoc_collection_command_simple

mongoc_collection_command_simple()
==================================

Synopsis
--------

.. code-block:: c

  bool
  mongoc_collection_command_simple (mongoc_collection_t *collection,
                                    const bson_t *command,
                                    const mongoc_read_prefs_t *read_prefs,
                                    bson_t *reply,
                                    bson_error_t *error);

Parameters
----------

* ``collection``: A :symbol:`mongoc_collection_t`.
* ``command``: A :symbol:`bson:bson_t` containing the command to execute.
* ``read_prefs``: An optional :symbol:`mongoc_read_prefs_t`. Otherwise, the command uses mode ``MONGOC_READ_PRIMARY``.
* ``reply``: A location to initialize a :symbol:`bson:bson_t`. This should be on the stack.
* ``error``: An optional location for a :symbol:`bson_error_t <errors>` or ``NULL``.

Description
-----------

This is a simplified version of :symbol:`mongoc_collection_command()` that returns the first result document in ``reply``. The collection's read preference, read concern, and write concern are not applied to the command. The parameter ``reply`` is initialized even upon failure to simplify memory management.

This function tries to unwrap an embedded error in the command when possible. The unwrapped error will be propagated via the ``error`` parameter. Additionally, the result document is set in ``reply``.

Errors
------

Errors are propagated via the ``error`` parameter.

Returns
-------

Returns ``true`` if successful. Returns ``false`` and sets ``error`` if there are invalid arguments or a server or network error.

This function does not check the server response for a write concern error or write concern timeout.

Example
-------

The following is an example of executing the collection stats command.

.. code-block:: c

  #include <mongoc.h>
  #include <bcon.h>
  #include <stdio.h>

  static void
  print_collection_stats (mongoc_collection_t *collection)
  {
     bson_error_t error;
     const char *name;
     bson_t *cmd;
     bson_t reply;

     name = mongoc_collection_get_name (collection);
     cmd = BCON_NEW ("collStats", BCON_UTF8 (name));

     if (mongoc_collection_command_simple (
            collection, cmd, NULL, &reply, &error)) {
        str = bson_as_canonical_extended_json (&reply, NULL);
        printf ("%s\n", str);
        bson_free (str);
     } else {
        fprintf (stderr, "%s\n", error.message);
     }

     bson_destroy (&reply);
     bson_destroy (cmd);
  }

