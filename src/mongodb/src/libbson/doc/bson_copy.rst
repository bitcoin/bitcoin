:man_page: bson_copy

bson_copy()
===========

Synopsis
--------

.. code-block:: c

  bson_t *
  bson_copy (const bson_t *bson);

Parameters
----------

* ``bson``: A :symbol:`bson_t`.

Description
-----------

The :symbol:`bson_copy()` function shall copy the contents of a bson document into a new :symbol:`bson_t`.

The resulting :symbol:`bson_t` should be freed with :symbol:`bson_destroy()`.

Returns
-------

A newly allocated :symbol:`bson_t` that should be freed with :symbol:`bson_destroy()`.

