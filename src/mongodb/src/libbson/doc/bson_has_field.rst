:man_page: bson_has_field

bson_has_field()
================

Synopsis
--------

.. code-block:: c

  bool
  bson_has_field (const bson_t *bson, const char *key);

Parameters
----------

* ``bson``: A :symbol:`bson_t`.
* ``key``: A string containing the name of the field to check for.

Description
-----------

Checks to see if key contains an element named ``key``. This also accepts "dotkey" notation such as "a.b.c.d".

Returns
-------

true if ``key`` was found within ``bson``; otherwise false.

