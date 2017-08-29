:man_page: mongoc_collection_stats

mongoc_collection_stats()
=========================

Synopsis
--------

.. code-block:: c

  bool
  mongoc_collection_stats (mongoc_collection_t *collection,
                           const bson_t *options,
                           bson_t *reply,
                           bson_error_t *error);

Parameters
----------

* ``collection``: A :symbol:`mongoc_collection_t`.
* ``options``: An optional :symbol:`bson:bson_t` containing extra options to pass to the ``collStats`` command.
* ``reply``: An uninitialized :symbol:`bson:bson_t` to store the result.
* ``error``: An optional location for a :symbol:`bson_error_t <errors>` or ``NULL``.

Description
-----------

Run the ``collStats`` command to retrieve statistics about the collection.

The command uses the :symbol:`mongoc_read_prefs_t` set on ``collection``.

Errors
------

Errors are propagated via the ``error`` parameter.

Returns
-------

Returns ``true`` if successful. Returns ``false`` and sets ``error`` if there are invalid arguments or a server or network error.

``reply`` is always initialized and must be freed with :symbol:`bson:bson_destroy`.

