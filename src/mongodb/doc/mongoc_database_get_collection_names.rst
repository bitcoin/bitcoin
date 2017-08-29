:man_page: mongoc_database_get_collection_names

mongoc_database_get_collection_names()
======================================

Synopsis
--------

.. code-block:: c

  char **
  mongoc_database_get_collection_names (mongoc_database_t *database,
                                        bson_error_t *error);

Fetches a ``NULL`` terminated array of ``NULL-byte`` terminated ``char*`` strings containing the names of all of the collections in ``database``.

Parameters
----------

* ``database``: A :symbol:`mongoc_database_t`.
* ``error``: An optional location for a :symbol:`bson_error_t <errors>` or ``NULL``.

Errors
------

Errors are propagated via the ``error`` parameter.

Returns
-------

A ``NULL`` terminated array of ``NULL`` terminated ``char*`` strings that should be freed with :symbol:`bson:bson_strfreev()`. Upon failure, ``NULL`` is returned and ``error`` is set.

Examples
--------

.. code-block:: c

  {
     bson_error_t error;
     char **strv;
     unsigned i;

     if ((strv = mongoc_database_get_collection_names (database, &error))) {
        for (i = 0; strv[i]; i++)
           printf ("%s\n", strv[i]);
        bson_strfreev (strv);
     } else {
        fprintf (stderr, "Command failed: %s\n", error.message);
     }
  }

