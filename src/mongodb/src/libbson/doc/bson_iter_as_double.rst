:man_page: bson_iter_as_double

bson_iter_as_double()
=====================

Synopsis
--------

.. code-block:: c

  bool
  bson_iter_as_double (const bson_iter_t *iter);

Parameters
----------

* ``iter``: A :symbol:`bson_iter_t`.

Description
-----------

Fetches the current field as if it were a double.

``bson_iter_as_double()`` will cast the following types to double

* BSON_TYPE_BOOL
* BSON_TYPE_DOUBLE
* BSON_TYPE_INT32
* BSON_TYPE_INT64

Any other value will return 0.

Returns
-------

The value type casted to double.

