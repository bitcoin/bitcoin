:man_page: bson_iter_as_int64

bson_iter_as_int64()
====================

Synopsis
--------

.. code-block:: c

  int64_t
  bson_iter_as_int64 (const bson_iter_t *iter);

Parameters
----------

* ``iter``: A :symbol:`bson_iter_t`.

Description
-----------

The ``bson_iter_as_int64()`` function shall return the contents of the current element as if it were a BSON_TYPE_INT64 element. The currently supported casts include:

* BSON_TYPE_BOOL
* BSON_TYPE_DOUBLE
* BSON_TYPE_INT32
* BSON_TYPE_INT64

Returns
-------

A 64-bit signed integer.

