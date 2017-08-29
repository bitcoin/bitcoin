:man_page: mongoc_database_get_collection

mongoc_database_get_collection()
================================

Synopsis
--------

.. code-block:: c

  mongoc_collection_t *
  mongoc_database_get_collection (mongoc_database_t *database, const char *name);

Allocates a new :symbol:`mongoc_collection_t` structure for the collection named ``name`` in ``database``.

Returns
-------

A newly allocated :symbol:`mongoc_collection_t` that should be freed with :symbol:`mongoc_collection_destroy()`.

