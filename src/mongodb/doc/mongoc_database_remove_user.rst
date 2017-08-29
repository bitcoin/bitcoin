:man_page: mongoc_database_remove_user

mongoc_database_remove_user()
=============================

Synopsis
--------

.. code-block:: c

  bool
  mongoc_database_remove_user (mongoc_database_t *database,
                               const char *username,
                               bson_error_t *error);

This function removes the user named ``username`` from ``database``.

Parameters
----------

* ``database``: A :symbol:`mongoc_database_t`.
* ``username``: A string containing the username of the user to remove.
* ``error``: An optional location for a :symbol:`bson_error_t <errors>` or ``NULL``.

Errors
------

Errors are propagated via the ``error`` parameter. This could include socket errors or others if the current user is not authorized to perform the command.

Returns
-------

Returns ``true`` if successful. Returns ``false`` and sets ``error`` if there are invalid arguments or a server or network error.

