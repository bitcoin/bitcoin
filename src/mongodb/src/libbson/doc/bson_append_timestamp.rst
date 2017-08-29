:man_page: bson_append_timestamp

bson_append_timestamp()
=======================

Synopsis
--------

.. code-block:: c

  #define BSON_APPEND_TIMESTAMP(b, key, val, inc) \
     bson_append_timestamp (b, key, (int) strlen (key), val, inc)

  bool
  bson_append_timestamp (bson_t *bson,
                         const char *key,
                         int key_length,
                         uint32_t timestamp,
                         uint32_t increment);

Parameters
----------

* ``bson``: A :symbol:`bson_t`.
* ``key``: An ASCII C string containing the name of the field.
* ``key_length``: The length of ``key`` in bytes, or -1 to determine the length with ``strlen()``.
* ``timestamp``: A uint32_t.
* ``increment``: A uint32_t.

Description
-----------

This function is not similar in functionality to :symbol:`bson_append_date_time()`. Timestamp elements are different in that they include only second precision and an increment field.

They are primarily used for intra-MongoDB server communication.

The :symbol:`bson_append_timestamp()` function shall append a new element of type BSON_TYPE_TIMESTAMP.

Returns
-------

Returns ``true`` if the operation was applied successfully. The function will fail if appending the value grows ``bson`` larger than INT32_MAX.
