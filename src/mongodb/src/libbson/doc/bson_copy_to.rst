:man_page: bson_copy_to

bson_copy_to()
==============

Synopsis
--------

.. code-block:: c

  void
  bson_copy_to (const bson_t *src, bson_t *dst);

Parameters
----------

* ``src``: A :symbol:`bson_t`.
* ``dst``: A :symbol:`bson_t`.

Description
-----------

The :symbol:`bson_copy_to()` function shall initialize ``dst`` with a copy of the contents of ``src``.

``dst`` *MUST* be an uninitialized :symbol:`bson_t` to avoid leaking memory.

