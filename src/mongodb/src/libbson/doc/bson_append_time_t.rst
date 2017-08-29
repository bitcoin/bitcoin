:man_page: bson_append_time_t

bson_append_time_t()
====================

Synopsis
--------

.. code-block:: c

  #define BSON_APPEND_TIME_T(b, key, val) \
     bson_append_time_t (b, key, (int) strlen (key), val)

  bool
  bson_append_time_t (bson_t *bson,
                      const char *key,
                      int key_length,
                      time_t value);

Parameters
----------

* ``bson``: A :symbol:`bson_t`.
* ``key``: An ASCII C string containing the name of the field.
* ``key_length``: The length of ``key`` in bytes, or -1 to determine the length with ``strlen()``.
* ``value``: A time_t.

Description
-----------

The :symbol:`bson_append_time_t()` function is a helper that takes a ``time_t`` instead of milliseconds since the UNIX epoch.

Returns
-------

Returns ``true`` if the operation was applied successfully. The function will fail if appending the value grows ``bson`` larger than INT32_MAX.
