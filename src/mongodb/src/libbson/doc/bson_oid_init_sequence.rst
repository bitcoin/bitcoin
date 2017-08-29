:man_page: bson_oid_init_sequence

bson_oid_init_sequence()
========================

Synopsis
--------

.. code-block:: c

  void
  bson_oid_init_sequence (bson_oid_t *oid, bson_context_t *context);

Parameters
----------

* ``oid``: A :symbol:`bson_oid_t`.
* ``context``: An optional :symbol:`bson_context_t`.

Description
-----------

Generates a new ObjectID using the 64-bit sequence.

.. warning::

  This form of ObjectID is generally used by MongoDB replica peers only.

