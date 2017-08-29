:man_page: bson_equal

bson_equal()
============

Synopsis
--------

.. code-block:: c

  bool
  bson_equal (const bson_t *bson, const bson_t *other);

Parameters
----------

* ``bson``: A :symbol:`bson_t`.
* ``other``: A :symbol:`bson_t`.

Description
-----------

The :symbol:`bson_equal()` function shall return true if both documents are equal.

Returns
-------

true if both documents are equal.

