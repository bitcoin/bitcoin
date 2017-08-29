:man_page: bson_iter_double

bson_iter_double()
==================

Synopsis
--------

.. code-block:: c

  #define BSON_ITER_HOLDS_DOUBLE(iter) \
     (bson_iter_type ((iter)) == BSON_TYPE_DOUBLE)

  double
  bson_iter_double (const bson_iter_t *iter);

Parameters
----------

* ``iter``: A :symbol:`bson_iter_t`.

Description
-----------

Fetches the contents of a BSON_TYPE_DOUBLE field.

Returns
-------

A double.

