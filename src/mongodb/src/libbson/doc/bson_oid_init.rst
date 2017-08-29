:man_page: bson_oid_init

bson_oid_init()
===============

Synopsis
--------

.. code-block:: c

  void
  bson_oid_init (bson_oid_t *oid, bson_context_t *context);

Parameters
----------

* ``oid``: A :symbol:`bson_oid_t`.
* ``context``: An *optional* :symbol:`bson_context_t` or NULL.

Description
-----------

Generates a new :symbol:`bson_oid_t` using either ``context`` or the default :symbol:`bson_context_t`.

