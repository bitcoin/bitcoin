:man_page: bson_append_double

bson_append_double()
====================

Synopsis
--------

.. code-block:: c

  #define BSON_APPEND_DOUBLE(b, key, val) \
     bson_append_double (b, key, (int) strlen (key), val)

  bool
  bson_append_double (bson_t *bson,
                      const char *key,
                      int key_length,
                      double value);

Parameters
----------

* ``bson``: A :symbol:`bson_t`.
* ``key``: An ASCII C string containing the name of the field.
* ``key_length``: The length of ``key`` in bytes, or -1 to determine the length with ``strlen()``.
* ``value``: A double value to append.

Description
-----------

The :symbol:`bson_append_double()` function shall append a new element to a bson document of type ``double``.

Returns
-------

Returns ``true`` if the operation was applied successfully. The function will fail if appending ``value`` grows ``bson`` larger than INT32_MAX.
