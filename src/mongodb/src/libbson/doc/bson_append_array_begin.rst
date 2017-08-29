:man_page: bson_append_array_begin

bson_append_array_begin()
=========================

Synopsis
--------

.. code-block:: c

  #define BSON_APPEND_ARRAY_BEGIN(b, key, child) \
     bson_append_array_begin (b, key, (int) strlen (key), child)

  bool
  bson_append_array_begin (bson_t *bson,
                           const char *key,
                           int key_length,
                           bson_t *child);

Parameters
----------

* ``bson``: A :symbol:`bson_t`.
* ``key``: A string containing the name for the key.
* ``key_length``: The length of ``key`` or -1 to call ``strlen()``.
* ``child``: A :symbol:`bson_t`.

Description
-----------

The :symbol:`bson_append_array_begin()` function shall begin appending an array field to ``bson``. This allows for incrementally building a sub-array. Doing so will generally yield better performance as you will serialize to a single buffer. When done building the sub-array, the caller *MUST* call :symbol:`bson_append_array_end()`.

For generating array element keys, see :symbol:`bson_uint32_to_string`.

Returns
-------

Returns ``true`` if the operation was applied successfully. The function will fail if appending the array grows ``bson`` larger than INT32_MAX.

