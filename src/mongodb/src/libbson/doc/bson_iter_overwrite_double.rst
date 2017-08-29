:man_page: bson_iter_overwrite_double

bson_iter_overwrite_double()
============================

Synopsis
--------

.. code-block:: c

  void
  bson_iter_overwrite_double (bson_iter_t *iter, double value);

Parameters
----------

* ``iter``: A :symbol:`bson_iter_t`.
* ``value``: The new double value.

Description
-----------

The :symbol:`bson_iter_overwrite_double()` function shall overwrite the contents of a BSON_TYPE_DOUBLE element in place.

This may only be done when the underlying bson document allows mutation.

It is a programming error to call this function when ``iter`` is not obvserving an element of type BSON_TYPE_DOUBLE.

