:man_page: bson_value_destroy

bson_value_destroy()
====================

Synopsis
--------

.. code-block:: c

  void
  bson_value_destroy (bson_value_t *value);

Parameters
----------

* ``value``: A :symbol:`bson_value_t`.

Description
-----------

Releases any resources associated with ``value``.

