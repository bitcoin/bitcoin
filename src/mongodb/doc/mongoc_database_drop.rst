:man_page: mongoc_database_drop

mongoc_database_drop()
======================

Synopsis
--------

.. code-block:: c

  bool
  mongoc_database_drop (mongoc_database_t *database, bson_error_t *error);

Parameters
----------

* ``database``: A :symbol:`mongoc_database_t`.
* ``error``: An optional location for a :symbol:`bson_error_t <errors>` or ``NULL``.

Description
-----------

For more information, see :symbol:`mongoc_database_drop_with_opts()`. This function is a thin wrapper, passing ``NULL`` in as :symbol:`opts <bson:bson_t>` parameter.

