:man_page: bson_oid_equal

bson_oid_equal()
================

Synopsis
--------

.. code-block:: c

  bool
  bson_oid_equal (const bson_oid_t *oid1, const bson_oid_t *oid2);

Parameters
----------

* ``oid1``: A :symbol:`bson_oid_t`.
* ``oid2``: A :symbol:`bson_oid_t`.

Description
-----------

Checks if two bson_oid_t contain the same bytes.

Returns
-------

true if they are equal, otherwise false.

