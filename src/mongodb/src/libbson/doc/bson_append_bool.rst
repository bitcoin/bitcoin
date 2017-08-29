:man_page: bson_append_bool

bson_append_bool()
==================

Synopsis
--------

.. code-block:: c

  #define BSON_APPEND_BOOL(b, key, val) \
     bson_append_bool (b, key, (int) strlen (key), val)

  bool
  bson_append_bool (bson_t *bson, const char *key, int key_length, bool value);

Parameters
----------

* ``bson``: A :symbol:`bson_t`.
* ``key``: The name of the field.
* ``key_length``: The length of ``key`` or -1 to use strlen().
* ``value``: true or false.

Description
-----------

The :symbol:`bson_append_bool()` function shall append a new element to ``bson`` containing the boolean provided.

Returns
-------

Returns ``true`` if the operation was applied successfully. The function will fail if appending the value grows ``bson`` larger than INT32_MAX.
