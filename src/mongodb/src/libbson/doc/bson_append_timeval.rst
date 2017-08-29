:man_page: bson_append_timeval

bson_append_timeval()
=====================

Synopsis
--------

.. code-block:: c

  #define BSON_APPEND_TIMEVAL(b, key, val) \
     bson_append_timeval (b, key, (int) strlen (key), val)

  bool
  bson_append_timeval (bson_t *bson,
                       const char *key,
                       int key_length,
                       struct timeval *value);

Parameters
----------

* ``bson``: A :symbol:`bson_t`.
* ``key``: An ASCII C string containing the name of the field.
* ``key_length``: The length of ``key`` in bytes, or -1 to determine the length with ``strlen()``.
* ``value``: A struct timeval.

Description
-----------

The :symbol:`bson_append_timeval()` function is a helper that takes a ``struct timeval`` instead of milliseconds since the UNIX epoch.

Returns
-------

Returns ``true`` if the operation was applied successfully. The function will fail if appending the value grows ``bson`` larger than INT32_MAX.
