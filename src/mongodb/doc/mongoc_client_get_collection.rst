:man_page: mongoc_client_get_collection

mongoc_client_get_collection()
==============================

Synopsis
--------

.. code-block:: c

  mongoc_collection_t *
  mongoc_client_get_collection (mongoc_client_t *client,
                                const char *db,
                                const char *collection);

Get a newly allocated :symbol:`mongoc_collection_t` for the collection named ``collection`` in the database named ``db``.

.. tip::

  Collections are automatically created on the MongoDB server upon insertion of the first document. There is no need to create a collection manually.

Parameters
----------

* ``client``: A :symbol:`mongoc_client_t`.
* ``db``: The name of the database containing the collection.
* ``collection``: The name of the collection.

Returns
-------

A newly allocated :symbol:`mongoc_collection_t` that should be freed with :symbol:`mongoc_collection_destroy()` when no longer in use.

