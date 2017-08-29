:man_page: mongoc_client_get_database

mongoc_client_get_database()
============================

Synopsis
--------

.. code-block:: c

  mongoc_database_t *
  mongoc_client_get_database (mongoc_client_t *client, const char *name);

Get a newly allocated :symbol:`mongoc_database_t` for the database named ``name``.

.. tip::

  Databases are automatically created on the MongoDB server upon insertion of the first document into a collection. There is no need to create a database manually.

Parameters
----------

* ``client``: A :symbol:`mongoc_client_t`.
* ``name``: The name of the database.

Returns
-------

A newly allocated :symbol:`mongoc_database_t <mongoc_client_t>` that should be freed with :symbol:`mongoc_database_destroy()` when no longer in use.

