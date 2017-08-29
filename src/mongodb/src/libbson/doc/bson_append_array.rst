:man_page: bson_append_array

bson_append_array()
===================

Synopsis
--------

.. code-block:: c

  #define BSON_APPEND_ARRAY(b, key, val) \
     bson_append_array (b, key, (int) strlen (key), val)

  bool
  bson_append_array (bson_t *bson,
                     const char *key,
                     int key_length,
                     const bson_t *array);

Parameters
----------

* ``bson``: A :symbol:`bson_t`.
* ``key``: An ASCII C string containing the name of the field.
* ``key_length``: The length of ``key`` in bytes, or -1 to determine the length with ``strlen()``.
* ``array``: A :symbol:`bson_t`.

Description
-----------

The :symbol:`bson_append_array()` function shall append ``child`` to ``bson`` using the specified key. The type of the field will be an array, but it is the responsibility of the caller to ensure that the keys of ``child`` are properly formatted with string keys such as "0", "1", "2" and so forth.

Returns
-------

Returns ``true`` if the operation was applied successfully. The function fails if appending the array grows ``bson`` larger than INT32_MAX.
