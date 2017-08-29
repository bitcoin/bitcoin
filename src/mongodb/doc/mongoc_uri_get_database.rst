:man_page: mongoc_uri_get_database

mongoc_uri_get_database()
=========================

Synopsis
--------

.. code-block:: c

  const char *
  mongoc_uri_get_database (const mongoc_uri_t *uri);

Parameters
----------

* ``uri``: A :symbol:`mongoc_uri_t`.

Description
-----------

Fetches the database portion of an URI if provided. This is the portion after the ``/`` but before the ``?``.

Returns
-------

A string which should not be modified or freed.

