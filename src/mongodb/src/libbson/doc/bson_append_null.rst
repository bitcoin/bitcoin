:man_page: bson_append_null

bson_append_null()
==================

Synopsis
--------

.. code-block:: c

  #define BSON_APPEND_NULL(b, key) bson_append_null (b, key, (int) strlen (key))

  bool
  bson_append_null (bson_t *bson, const char *key, int key_length);

Parameters
----------

* ``bson``: A :symbol:`bson_t`.
* ``key``: An ASCII C string containing the name of the field.
* ``key_length``: The length of ``key`` in bytes, or -1 to determine the length with ``strlen()``.

Description
-----------

The :symbol:`bson_append_null()` function shall append a new element to ``bson`` of type BSON_TYPE_NULL.

Returns
-------

Returns ``true`` if the operation was applied successfully. The function will fail if appending the value grows ``bson`` larger than INT32_MAX.
