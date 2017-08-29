:man_page: bson_iter_oid

bson_iter_oid()
===============

Synopsis
--------

.. code-block:: c

  #define BSON_ITER_HOLDS_OID(iter) (bson_iter_type ((iter)) == BSON_TYPE_OID)

  const bson_oid_t *
  bson_iter_oid (const bson_iter_t *iter);

Parameters
----------

* ``iter``: A :symbol:`bson_iter_t`.

Description
-----------

Fetches the :symbol:`bson_oid_t` for a BSON_TYPE_OID element. You should verify it is an element of type BSON_TYPE_OID before calling this function.

Returns
-------

A :symbol:`bson_oid_t` that should not be modified or freed.

