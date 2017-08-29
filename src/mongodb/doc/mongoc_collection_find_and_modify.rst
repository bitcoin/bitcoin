:man_page: mongoc_collection_find_and_modify

mongoc_collection_find_and_modify()
===================================

Synopsis
--------

.. code-block:: c

  bool
  mongoc_collection_find_and_modify (mongoc_collection_t *collection,
                                     const bson_t *query,
                                     const bson_t *sort,
                                     const bson_t *update,
                                     const bson_t *fields,
                                     bool _remove,
                                     bool upsert,
                                     bool _new,
                                     bson_t *reply,
                                     bson_error_t *error);

Parameters
----------

* ``collection``: A :symbol:`mongoc_collection_t`.
* ``query``: A :symbol:`bson:bson_t` containing the query to locate target document(s).
* ``sort``: A :symbol:`bson:bson_t` containing the sort order for ``query``.
* ``update``: A :symbol:`bson:bson_t` containing an update spec.
* ``fields``: An optional :symbol:`bson:bson_t` containing the fields to return or ``NULL``.
* ``_remove``: If the matching documents should be removed.
* ``upsert``: If an upsert should be performed.
* ``_new``: If the new version of the document should be returned.
* ``reply``: Optional pointer to an *uninitialized* :symbol:`bson:bson_t` that will be initialized with the result.
* ``error``: An optional location for a :symbol:`bson_error_t <errors>` or ``NULL``.

Description
-----------

Update and return an object.

This is a thin wrapper around the ``findAndModify`` command. Either ``update`` or ``_remove`` arguments are required.

See also: :symbol:`mongoc_collection_find_and_modify_with_opts`.

As of MongoDB 3.2, the :symbol:`mongoc_write_concern_t` specified on the :symbol:`mongoc_collection_t` will be used, if any.

Errors
------

Errors are propagated via the ``error`` parameter.

Returns
-------

Returns either the document before or after modification based on the ``_new`` parameter.

A write concern timeout or write concern error is considered a failure.

Example
-------

.. literalinclude:: ../examples/find-and-modify.c
   :language: c
   :caption: find-and-modify.c

