:man_page: bson_append_undefined

bson_append_undefined()
=======================

Synopsis
--------

.. code-block:: c

  #define BSON_APPEND_UNDEFINED(b, key) \
     bson_append_undefined (b, key, (int) strlen (key))

  bool
  bson_append_undefined (bson_t *bson, const char *key, int key_length);

Parameters
----------

* ``bson``: A :symbol:`bson_t`.
* ``key``: An ASCII C string containing the name of the field.
* ``key_length``: The length of ``key`` in bytes, or -1 to determine the length with ``strlen()``.

Description
-----------

The :symbol:`bson_append_undefined()` function shall append a new element to ``bson`` of type BSON_TYPE_UNDEFINED. Undefined is common in Javascript. However, this element type is deprecated and should not be used in new code.

Returns
-------

Returns ``true`` if the operation was applied successfully. The function will fail if appending the value grows ``bson`` larger than INT32_MAX.
