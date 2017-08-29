:man_page: bson_iter_overwrite_int32

bson_iter_overwrite_int32()
===========================

Synopsis
--------

.. code-block:: c

  void
  bson_iter_overwrite_int32 (bson_iter_t *iter, int32_t value);

Parameters
----------

* ``iter``: A :symbol:`bson_iter_t`.
* ``value``: A int32_t.

Description
-----------

The :symbol:`bson_iter_overwrite_int32()` function shall overwrite the contents of a BSON_TYPE_INT32 element in place.

This may only be done when the underlying bson document allows mutation.

It is a programming error to call this function when ``iter`` is not obvserving an element of type BSON_TYPE_BOOL.

