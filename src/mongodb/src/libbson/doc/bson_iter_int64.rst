:man_page: bson_iter_int64

bson_iter_int64()
=================

Synopsis
--------

.. code-block:: c

  #define BSON_ITER_HOLDS_INT64(iter) (bson_iter_type ((iter)) == BSON_TYPE_INT64)

  int64_t
  bson_iter_int64 (const bson_iter_t *iter);

Parameters
----------

* ``iter``: A :symbol:`bson_iter_t`.

Description
-----------

Fetches the value from a BSON_TYPE_INT64 field. You should verify that this is a BSON_TYPE_INT64 field before calling this function.

Returns
-------

A 64-bit integer.

