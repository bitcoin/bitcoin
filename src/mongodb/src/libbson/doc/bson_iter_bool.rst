:man_page: bson_iter_bool

bson_iter_bool()
================

Synopsis
--------

.. code-block:: c

  #define BSON_ITER_HOLDS_BOOL(iter) (bson_iter_type ((iter)) == BSON_TYPE_BOOL)

  bool
  bson_iter_bool (const bson_iter_t *iter);

Parameters
----------

* ``iter``: A :symbol:`bson_iter_t`.

Description
-----------

The ``bson_iter_bool()`` function shall return the boolean value of a BSON_TYPE_BOOL element. It is a programming error to call this function on an element other than BSON_TYPE_BOOL. You can check this with :symbol:`bson_iter_type()` or ``BSON_ITER_HOLDS_BOOL()``.

Returns
-------

Either ``true`` or ``false``.

