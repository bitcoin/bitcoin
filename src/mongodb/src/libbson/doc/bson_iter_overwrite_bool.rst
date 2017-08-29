:man_page: bson_iter_overwrite_bool

bson_iter_overwrite_bool()
==========================

Synopsis
--------

.. code-block:: c

  void
  bson_iter_overwrite_bool (bson_iter_t *iter, bool value);

Parameters
----------

* ``iter``: A :symbol:`bson_iter_t`.
* ``value``: A bool containing the new value.

Description
-----------

The ``bson_iter_overwrite_bool()`` function shall overwrite the contents of a BSON_TYPE_BOOL element in place.

This may only be done when the underlying bson document allows mutation.

It is a programming error to call this function when ``iter`` is not obvserving an element of type BSON_TYPE_BOOL.

