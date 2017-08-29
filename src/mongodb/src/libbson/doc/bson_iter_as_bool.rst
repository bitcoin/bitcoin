:man_page: bson_iter_as_bool

bson_iter_as_bool()
===================

Synopsis
--------

.. code-block:: c

  bool
  bson_iter_as_bool (const bson_iter_t *iter);

Parameters
----------

* ``iter``: A :symbol:`bson_iter_t`.

Description
-----------

Fetches the current field as if it were a boolean.

``bson_iter_as_bool()`` currently knows how to determine a boolean value from the following types:

* BSON_TYPE_BOOL
* BSON_TYPE_DOUBLE
* BSON_TYPE_INT32
* BSON_TYPE_INT64
* BSON_TYPE_NULL
* BSON_TYPE_UNDEFINED
* BSON_TYPE_UTF8

BSON_TYPE_UTF8 will always equate to ``true``.

Returns
-------

true if the field equates to non-zero.

