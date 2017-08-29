:man_page: bson_iter_dbpointer

bson_iter_dbpointer()
=====================

Synopsis
--------

.. code-block:: c

  void
  bson_iter_dbpointer (const bson_iter_t *iter,
                       uint32_t *collection_len,
                       const char **collection,
                       const bson_oid_t **oid);

Parameters
----------

* ``iter``: A :symbol:`bson_iter_t`.
* ``collection_len``: A location for the length of the collection name.
* ``collection``: A location for the collection name..
* ``oid``: A location for a :symbol:`bson_oid_t`.

Description
-----------

Fetches the contents of a BSON_TYPE_DBPOINTER element.

.. warning::

  The BSON_TYPE_DBPOINTER field type is deprecated by the BSON spec and should not be used in new code.

