:man_page: mongoc_collection_rename

mongoc_collection_rename()
==========================

Synopsis
--------

.. code-block:: c

  bool
  mongoc_collection_rename (mongoc_collection_t *collection,
                            const char *new_db,
                            const char *new_name,
                            bool drop_target_before_rename,
                            bson_error_t *error);

Parameters
----------

* ``collection``: A :symbol:`mongoc_collection_t`.
* ``new_db``: The name of the new database.
* ``new_name``: The new name for the collection.
* ``drop_target_before_rename``: If an existing collection matches the new name, drop it before the rename.
* ``error``: An optional location for a :symbol:`bson_error_t <errors>` or ``NULL``.

Description
-----------

For more information, see :symbol:`mongoc_collection_rename_with_opts()`. This function is a thin wrapper, passing ``NULL`` in as :symbol:`opts <bson:bson_t>` parameter.

