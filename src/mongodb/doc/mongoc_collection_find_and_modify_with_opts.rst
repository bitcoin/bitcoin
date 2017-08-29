:man_page: mongoc_collection_find_and_modify_with_opts

mongoc_collection_find_and_modify_with_opts()
=============================================

Synopsis
--------

.. code-block:: c

  bool
  mongoc_collection_find_and_modify_with_opts (
     mongoc_collection_t *collection,
     const bson_t *query,
     const mongoc_find_and_modify_opts_t *opts,
     bson_t *reply,
     bson_error_t *error);

Parameters
----------

* ``collection``: A :symbol:`mongoc_collection_t`.
* ``query``: A :symbol:`bson:bson_t` containing the query to locate target document(s).
* ``opts``: :symbol:`find and modify options <mongoc_find_and_modify_opts_t>`
* ``reply``: An optional location for a :symbol:`bson:bson_t` that will be initialized with the result or ``NULL``.
* ``error``: An optional location for a :symbol:`bson_error_t <errors>` or ``NULL``.

Description
-----------

Update and return an object.

Errors
------

Errors are propagated via the ``error`` parameter.

Returns
-------

Returns ``true`` if successful. Returns ``false`` and sets ``error`` if there are invalid arguments or a server or network error.

A write concern timeout or write concern error is considered a failure.

Example
-------

See the example code for :ref:`mongoc_find_and_modify_opts_t <mongoc_collection_find_and_modify_with_opts_example>`.

