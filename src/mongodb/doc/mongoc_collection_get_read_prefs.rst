:man_page: mongoc_collection_get_read_prefs

mongoc_collection_get_read_prefs()
==================================

Synopsis
--------

.. code-block:: c

  const mongoc_read_prefs_t *
  mongoc_collection_get_read_prefs (const mongoc_collection_t *collection);

Parameters
----------

* ``collection``: A :symbol:`mongoc_collection_t`.

Description
-----------

Fetches the default read preferences to use for ``collection``. Operations without specified read-preferences will default to this.

Returns
-------

A :symbol:`mongoc_read_prefs_t` that should not be modified or freed.

