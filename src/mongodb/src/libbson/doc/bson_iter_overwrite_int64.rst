:man_page: bson_iter_overwrite_int64

bson_iter_overwrite_int64()
===========================

Synopsis
--------

.. code-block:: c

  void
  bson_iter_overwrite_int64 (bson_iter_t *iter, int64_t value);

Parameters
----------

* ``iter``: A :symbol:`bson_iter_t`.
* ``value``: A int64_t.

Description
-----------

The :symbol:`bson_iter_overwrite_int64()` function shall overwrite the contents of a BSON_TYPE_INT64 element in place.

This may only be done when the underlying bson document allows mutation.

It is a programming error to call this function when ``iter`` is not obvserving an element of type BSON_TYPE_INT64.

