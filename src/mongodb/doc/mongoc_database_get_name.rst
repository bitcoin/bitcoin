:man_page: mongoc_database_get_name

mongoc_database_get_name()
==========================

Synopsis
--------

.. code-block:: c

  const char *
  mongoc_database_get_name (mongoc_database_t *database);

Fetches the name of the database.

Parameters
----------

* ``database``: A :symbol:`mongoc_database_t`.

Returns
-------

A string which should not be modified or freed.

