:man_page: bson_append_now_utc

bson_append_now_utc()
=====================

Synopsis
--------

.. code-block:: c

  bool
  bson_append_now_utc (bson_t *bson, const char *key, int key_length);

Parameters
----------

* ``bson``: A :symbol:`bson_t`.
* ``key``: An ASCII C string containing the name of the field.
* ``key_length``: The length of ``key`` in bytes, or -1 to determine the length with ``strlen()``.

Description
-----------

The :symbol:`bson_append_now_utc()` function is a helper to get the current date and time in UTC and append it to ``bson`` as a BSON_TYPE_DATE_TIME element.

This function calls :symbol:`bson_append_date_time()` internally.

Returns
-------

Returns ``true`` if the operation was applied successfully. The function will fail if appending the value grows ``bson`` larger than INT32_MAX.
