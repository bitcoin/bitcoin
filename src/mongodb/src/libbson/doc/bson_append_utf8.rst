:man_page: bson_append_utf8

bson_append_utf8()
==================

Synopsis
--------

.. code-block:: c

  #define BSON_APPEND_UTF8(b, key, val) \
     bson_append_utf8 (b, key, (int) strlen (key), val, (int) strlen (val))

  bool
  bson_append_utf8 (bson_t *bson,
                    const char *key,
                    int key_length,
                    const char *value,
                    int length);

Parameters
----------

* ``bson``: A :symbol:`bson_t`.
* ``key``: An ASCII C string containing the name of the field.
* ``key_length``: The length of ``key`` in bytes, or -1 to determine the length with ``strlen()``.
* ``value``: A UTF-8 encoded string.
* ``length``: The number of bytes in ``value`` excluding the trailing ``\0``, or -1 to determine the length with ``strlen()``.

Description
-----------

The :symbol:`bson_append_utf8()` function shall append a UTF-8 encoded string to ``bson``.

``value`` *MUST* be valid UTF-8.

Some UTF-8 implementations allow for ``\0`` to be contained within the string (excluding the termination ``\0``. This is allowed, but remember that it could cause issues with communicating with external systems that do not support it.

It is suggested to use modified UTF-8 which uses a 2 byte representation for embedded ``\0`` within the string. This will allow these UTF-8 encoded strings to used with many libc functions.

Returns
-------

Returns ``true`` if the operation was applied successfully. The function will fail if appending the value grows ``bson`` larger than INT32_MAX.
