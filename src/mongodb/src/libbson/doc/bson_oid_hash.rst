:man_page: bson_oid_hash

bson_oid_hash()
===============

Synopsis
--------

.. code-block:: c

  uint32_t
  bson_oid_hash (const bson_oid_t *oid);

Parameters
----------

* ``oid``: A :symbol:`bson_oid_t`.

Description
-----------

Generates a hash code for ``oid`` suitable for a hashtable.

Returns
-------

A 32-bit hash code.

