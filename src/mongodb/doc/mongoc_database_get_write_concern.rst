:man_page: mongoc_database_get_write_concern

mongoc_database_get_write_concern()
===================================

Synopsis
--------

.. code-block:: c

  const mongoc_write_concern_t *
  mongoc_database_get_write_concern (const mongoc_database_t *database);

This function retrieves the default :symbol:`mongoc_write_concern_t` to use with ``database`` as configured by the client.

Parameters
----------

* ``database``: A :symbol:`mongoc_database_t`.

Returns
-------

A :symbol:`mongoc_write_concern_t` that should not be modified or freed.

