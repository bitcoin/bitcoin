:man_page: bson_iter_value

bson_iter_value()
=================

Synopsis
--------

.. code-block:: c

  const bson_value_t *
  bson_iter_value (bson_iter_t *iter);

Parameters
----------

* ``iter``: A :symbol:`bson_iter_t`.

Description
-----------

Fetches the value for the currently observed type as a boxed type. This allows passing around the value without knowing the type at runtime.

Returns
-------

A :symbol:`bson_value_t` that should not be modified or freed.

