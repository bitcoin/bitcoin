:man_page: bson_append_decimal128

bson_append_decimal128()
========================

Synopsis
--------

.. code-block:: c

  #define BSON_APPEND_DECIMAL128(b, key, val) \
     bson_append_decimal128 (b, key, (int) strlen (key), val)

  bool
  bson_append_decimal128 (bson_t *bson,
                          const char *key,
                          int key_length,
                          const bson_decimal128_t *value);

Parameters
----------

* ``bson``: A :symbol:`bson_t`.
* ``key``: An ASCII C string containing the name of the field.
* ``key_length``: The length of ``key`` in bytes, or -1 to determine the length with ``strlen()``.
* ``value``: A :symbol:`bson_decimal128_t`.

Description
-----------

The :symbol:`bson_append_decimal128()` function shall append a new element to ``bson`` containing a Decimal 128.

Returns
-------

Returns ``true`` if the operation was applied successfully. The function will fail if appending ``value`` grows ``bson`` larger than INT32_MAX.
