:man_page: bson_iter_overwrite_decimal128

bson_iter_overwrite_decimal128()
================================

Synopsis
--------

.. code-block:: c

  bool
  bson_iter_overwrite_decimal128 (bson_iter_t *iter, bson_decimal128_t *value);

Parameters
----------

* ``iter``: A :symbol:`bson_iter_t`.
* ``value``: The new Decimal128 value.

Description
-----------

The :symbol:`bson_iter_overwrite_decimal128()` function shall overwrite the contents of a BSON_TYPE_DECIMAL128 element in place.

This may only be done when the underlying bson document allows mutation.

It is a programming error to call this function when ``iter`` is not obvserving an element of type BSON_TYPE_DECIMAL128.

