:man_page: bson_append_binary

bson_append_binary()
====================

Synopsis
--------

.. code-block:: c

  #define BSON_APPEND_BINARY(b, key, subtype, val, len) \
     bson_append_binary (b, key, (int) strlen (key), subtype, val, len)

  bool
  bson_append_binary (bson_t *bson,
                      const char *key,
                      int key_length,
                      bson_subtype_t subtype,
                      const uint8_t *binary,
                      uint32_t length);

Parameters
----------

* ``bson``: A :symbol:`bson_t`.
* ``key``: The key name.
* ``key_length``: The length of ``key`` in bytes or -1 to use strlen().
* ``subtype``: A bson_subtype_t indicating the binary subtype.
* ``binary``: A buffer to embed as binary data.
* ``length``: The length of ``buffer`` in bytes.

Description
-----------

The :symbol:`bson_append_binary()` function shall append a new element to ``bson`` containing the binary data provided.

Returns
-------

Returns ``true`` if the operation was applied successfully. The function will fail if appending ``binary`` grows ``bson`` larger than INT32_MAX.
