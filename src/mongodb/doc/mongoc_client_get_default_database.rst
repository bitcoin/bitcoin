:man_page: mongoc_client_get_default_database

mongoc_client_get_default_database()
====================================

Synopsis
--------

.. code-block:: c

  mongoc_database_t *
  mongoc_client_get_default_database (mongoc_client_t *client);

Get the database named in the MongoDB connection URI, or ``NULL`` if the URI specifies none.

Useful when you want to choose which database to use based only on the URI in a configuration file.

Parameters
----------

* ``client``: A :symbol:`mongoc_client_t`.

Returns
-------

A newly allocated :symbol:`mongoc_database_t <mongoc_client_t>` that should be freed with :symbol:`mongoc_database_destroy()`.

Example
-------

.. code-block:: c
  :caption: Default Database Example

  /* default database is "db_name" */
  mongoc_client_t *client = mongoc_client_new ("mongodb://host/db_name");
  mongoc_database_t *db = mongoc_client_get_default_database (client);

  assert (!strcmp ("db_name", mongoc_database_get_name (db)));

  mongoc_database_destroy (db);
  mongoc_client_destroy (client);

  /* no default database */
  client = mongoc_client_new ("mongodb://host/");
  db = mongoc_client_get_default_database (client);

  assert (!db);

  mongoc_client_destroy (client);

