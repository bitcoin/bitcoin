:man_page: bson_iter_decimal128

bson_iter_decimal128()
======================

Synopsis
--------

.. code-block:: c

  #define BSON_ITER_HOLDS_DECIMAL128(iter) \
     (bson_iter_type ((iter)) == BSON_TYPE_DECIMAL128)

  bool
  bson_iter_decimal128 (const bson_iter_t *iter, /* IN */
                        bson_decimal128_t *dec); /* OUT */

Parameters
----------

* ``iter``: A :symbol:`bson_iter_t`.
* ``dec``: A location for a :symbol:`bson_decimal128_t`.

Description
-----------

Fetches the value from a BSON_TYPE_DECIMAL128 field. You should verify that this is a BSON_TYPE_DECIMAL128 field before calling this function.

Returns
-------

true if type was BSON_TYPE_DECIMAL128, otherwise false.

