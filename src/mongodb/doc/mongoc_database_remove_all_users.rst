:man_page: mongoc_database_remove_all_users

mongoc_database_remove_all_users()
==================================

Synopsis
--------

.. code-block:: c

  bool
  mongoc_database_remove_all_users (mongoc_database_t *database,
                                    bson_error_t *error);

This function will remove all users configured to access ``database``.

Parameters
----------

* ``database``: A :symbol:`mongoc_database_t`.
* ``error``: An optional location for a :symbol:`bson_error_t <errors>` or ``NULL``.

Errors
------

Errors are propagated via the ``error`` parameter. This may fail if there are socket errors or the current user is not authorized to perform the given command.

Returns
-------

Returns ``true`` if successful. Returns ``false`` and sets ``error`` if there are invalid arguments or a server or network error.

