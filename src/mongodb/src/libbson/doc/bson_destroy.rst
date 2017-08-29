:man_page: bson_destroy

bson_destroy()
==============

Synopsis
--------

.. code-block:: c

  void
  bson_destroy (bson_t *bson);

Parameters
----------

* ``bson``: A :symbol:`bson_t`.

Description
-----------

The :symbol:`bson_destroy()` function shall free an allocated :symbol:`bson_t` structure.

This function should always be called when you are done with a :symbol:`bson_t` unless otherwise specified.

