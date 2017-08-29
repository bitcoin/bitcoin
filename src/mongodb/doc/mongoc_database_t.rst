:man_page: mongoc_database_t

mongoc_database_t
=================

MongoDB Database Abstraction

Synopsis
--------

.. code-block:: c

  typedef struct _mongoc_database_t mongoc_database_t;

``mongoc_database_t`` provides access to a MongoDB database. This handle is useful for actions a particular database object. It *is not* a container for :symbol:`mongoc_collection_t` structures.

Read preferences and write concerns are inherited from the parent client. They can be overridden with :symbol:`mongoc_database_set_read_prefs()` and :symbol:`mongoc_database_set_write_concern()`.

.. only:: html

  Functions
  ---------

  .. toctree::
    :titlesonly:
    :maxdepth: 1

    mongoc_database_add_user
    mongoc_database_command
    mongoc_database_command_simple
    mongoc_database_copy
    mongoc_database_create_collection
    mongoc_database_destroy
    mongoc_database_drop
    mongoc_database_drop_with_opts
    mongoc_database_find_collections
    mongoc_database_get_collection
    mongoc_database_get_collection_names
    mongoc_database_get_name
    mongoc_database_get_read_concern
    mongoc_database_get_read_prefs
    mongoc_database_get_write_concern
    mongoc_database_has_collection
    mongoc_database_read_command_with_opts
    mongoc_database_read_write_command_with_opts
    mongoc_database_remove_all_users
    mongoc_database_remove_user
    mongoc_database_set_read_concern
    mongoc_database_set_read_prefs
    mongoc_database_set_write_concern
    mongoc_database_write_command_with_opts

Examples
--------

.. code-block:: c

  #include <mongoc.h>

  int
  main (int argc, char *argv[])
  {
     mongoc_database_t *database;
     mongoc_client_t *client;

     mongoc_init ();

     client = mongoc_client_new ("mongodb://localhost/");
     database = mongoc_client_get_database (client, "test");

     mongoc_database_destroy (database);
     mongoc_client_destroy (client);

     mongoc_cleanup ();

     return 0;
  }

