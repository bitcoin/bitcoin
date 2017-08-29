:man_page: mongoc_uri_set_database

mongoc_uri_set_database()
=========================

Synopsis
--------

.. code-block:: c

  bool
  mongoc_uri_set_database (mongoc_uri_t *uri, const char *database);

Parameters
----------

* ``uri``: A :symbol:`mongoc_uri_t`.
* ``database``: The new database name.

Description
-----------

Sets the URI's database, after the URI has been parsed from a string.

The driver authenticates to this database if the connection string includes authentication credentials. This database is also the return value of :symbol:`mongoc_client_get_default_database`.

Returns
-------

Returns false if the option cannot be set, for example if ``database`` is not valid UTF-8.

