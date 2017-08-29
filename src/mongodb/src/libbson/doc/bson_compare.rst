:man_page: bson_compare

bson_compare()
==============

Synopsis
--------

.. code-block:: c

  int
  bson_compare (const bson_t *bson, const bson_t *other);

Parameters
----------

* ``bson``: A :symbol:`bson_t`.
* ``other``: A :symbol:`bson_t`.

Description
-----------

The :symbol:`bson_compare()` function shall compare two bson documents for equality.

This can be useful in conjunction with _qsort()_.

If equal, 0 is returned.

.. tip::

  This function uses _memcmp()_ internally, so the semantics are the same.

Returns
-------

less than zero, zero, or greater than zero in ``qsort()`` style.

