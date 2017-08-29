:man_page: bson_append_minkey

bson_append_minkey()
====================

Synopsis
--------

.. code-block:: c

  #define BSON_APPEND_MINKEY(b, key) \
     bson_append_minkey (b, key, (int) strlen (key))

  bool
  bson_append_minkey (bson_t *bson, const char *key, int key_length);

Parameters
----------

* ``bson``: A :symbol:`bson_t`.
* ``key``: An ASCII C string containing the name of the field.
* ``key_length``: The length of ``key`` in bytes, or -1 to determine the length with ``strlen()``.

Description
-----------

The :symbol:`bson_append_minkey()` function shall append an element of type BSON_TYPE_MINKEY to a bson document. This is primarily used in queries and unlikely to be used when storing a document to MongoDB.

Returns
-------

Returns ``true`` if the operation was applied successfully. The function will fail if appending the value grows ``bson`` larger than INT32_MAX.
