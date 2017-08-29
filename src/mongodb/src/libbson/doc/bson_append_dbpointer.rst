:man_page: bson_append_dbpointer

bson_append_dbpointer()
=======================

Synopsis
--------

.. code-block:: c

  #define BSON_APPEND_DBPOINTER(b, key, coll, oid) \
     bson_append_dbpointer (b, key, (int) strlen (key), coll, oid)

  bool
  bson_append_dbpointer (bson_t *bson,
                         const char *key,
                         int key_length,
                         const char *collection,
                         const bson_oid_t *oid);

Parameters
----------

* ``bson``: A :symbol:`bson_t`.
* ``key``: An ASCII C string containing the name of the field.
* ``key_length``: The length of ``key`` in bytes, or -1 to determine the length with ``strlen()``.
* ``collection``: The target collection name.
* ``oid``: The target document identifier.

Description
-----------

.. warning::

  The dbpointer field type is *DEPRECATED* and should only be used when interacting with legacy systems.

Returns
-------

Returns ``true`` if the operation was applied successfully. The function will fail if appending the array grows ``bson`` larger than INT32_MAX.
