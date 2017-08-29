:man_page: bson_oid_compare

bson_oid_compare()
==================

Synopsis
--------

.. code-block:: c

  int
  bson_oid_compare (const bson_oid_t *oid1, const bson_oid_t *oid2);

Parameters
----------

* ``oid1``: A :symbol:`bson_oid_t`.
* ``oid2``: A :symbol:`bson_oid_t`.

Description
-----------

The :symbol:`bson_oid_compare()` function shall return a qsort() style value of a lexicographical sort of _oid1_ and _oid2_.

Returns
-------

less than 0, 0, or greater than 0 based on the comparison.

