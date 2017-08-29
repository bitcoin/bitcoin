:man_page: bson_count_keys

bson_count_keys()
=================

Synopsis
--------

.. code-block:: c

  uint32_t
  bson_count_keys (const bson_t *bson);

Parameters
----------

* ``bson``: A :symbol:`bson_t`.

Description
-----------

The :symbol:`bson_count_keys()` function shall count the number of elements within ``bson``.

Returns
-------

A positive integer or zero.

