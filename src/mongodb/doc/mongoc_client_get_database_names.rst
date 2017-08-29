:man_page: mongoc_client_get_database_names

mongoc_client_get_database_names()
==================================

Synopsis
--------

.. code-block:: c

  char **
  mongoc_client_get_database_names (mongoc_client_t *client, bson_error_t *error);

This function queries the MongoDB server for a list of known databases.

Parameters
----------

* ``client``: A :symbol:`mongoc_client_t`.
* ``error``: An optional location for a :symbol:`bson_error_t <errors>` or ``NULL``.

Errors
------

Errors are propagated via the ``error`` parameter.

Returns
-------

A ``NULL`` terminated vector of ``NULL-byte`` terminated strings. The result should be freed with :symbol:`bson:bson_strfreev()`.

``NULL`` is returned upon failure and ``error`` is set.

Examples
--------

.. code-block:: c

  {
     bson_error_t error;
     char **strv;
     unsigned i;

     if ((strv = mongoc_client_get_database_names (client, &error))) {
        for (i = 0; strv[i]; i++)
           printf ("%s\n", strv[i]);
        bson_strfreev (strv);
     } else {
        fprintf (stderr, "Command failed: %s\n", error.message);
     }
  }

