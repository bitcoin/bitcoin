:man_page: bson_append_int32

bson_append_int32()
===================

Synopsis
--------

.. code-block:: c

  #define BSON_APPEND_INT32(b, key, val) \
     bson_append_int32 (b, key, (int) strlen (key), val)

  bool
  bson_append_int32 (bson_t *bson,
                     const char *key,
                     int key_length,
                     int32_t value);

Parameters
----------

* ``bson``: A :symbol:`bson_t`.
* ``key``: An ASCII C string containing the name of the field.
* ``key_length``: The length of ``key`` in bytes, or -1 to determine the length with ``strlen()``.
* ``value``: An int32_t.

Description
-----------

The :symbol:`bson_append_int32()` function shall append a new element to ``bson`` containing a 32-bit signed integer.

Returns
-------

Returns ``true`` if the operation was applied successfully. The function will fail if appending ``value`` grows ``bson`` larger than INT32_MAX.
