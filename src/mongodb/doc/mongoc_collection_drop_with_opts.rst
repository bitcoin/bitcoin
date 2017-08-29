:man_page: mongoc_collection_drop_with_opts

mongoc_collection_drop_with_opts()
==================================

Synopsis
--------

.. code-block:: c

  bool
  mongoc_collection_drop_with_opts (mongoc_collection_t *collection,
                                    bson_t *opts,
                                    bson_error_t *error);

Parameters
----------

* ``collection``: A :symbol:`mongoc_collection_t`.
* ``opts``: A :symbol:`bson:bson_t` or ``NULL``.
* ``error``: An optional location for a :symbol:`bson_error_t <errors>` or ``NULL``.

Description
-----------

This function requests that a ``collection`` be dropped, including all indexes associated with the ``collection``.

If no write concern is provided in ``opts``, the collection's write concern is used.

Errors
------

Errors are propagated via the ``error`` parameter.

Returns
-------

Returns true if the collection was successfully dropped. Returns ``false`` and sets ``error`` if there are invalid arguments or a server or network error.


