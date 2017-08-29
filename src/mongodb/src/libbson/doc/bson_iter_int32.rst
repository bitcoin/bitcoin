:man_page: bson_iter_int32

bson_iter_int32()
=================

Synopsis
--------

.. code-block:: c

  #define BSON_ITER_HOLDS_INT32(iter) (bson_iter_type ((iter)) == BSON_TYPE_INT32)

  int32_t
  bson_iter_int32 (const bson_iter_t *iter);

Parameters
----------

* ``iter``: A :symbol:`bson_iter_t`.

Description
-----------

Fetches the value from a BSON_TYPE_INT32 element. You should verify that the field is a BSON_TYPE_INT32 field before calling this function.

Returns
-------

A 32-bit integer.

