:man_page: bson_oid_init_from_data

bson_oid_init_from_data()
=========================

Synopsis
--------

.. code-block:: c

  void
  bson_oid_init_from_data (bson_oid_t *oid, const uint8_t *data);

Parameters
----------

* ``oid``: A :symbol:`bson_oid_t`.
* ``data``: A buffer containing 12 bytes for the oid.

Description
-----------

Initializes a :symbol:`bson_oid_t` using the raw buffer provided.

