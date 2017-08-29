:man_page: bson_iter_array

bson_iter_array()
=================

Synopsis
--------

.. code-block:: c

  #define BSON_ITER_HOLDS_ARRAY(iter) (bson_iter_type ((iter)) == BSON_TYPE_ARRAY)

  void
  bson_iter_array (const bson_iter_t *iter,
                   uint32_t *array_len,
                   const uint8_t **array);

Parameters
----------

* ``iter``: A :symbol:`bson_iter_t`.
* ``array_len``: A location for the buffer length.
* ``array``: A location for the immutable buffer.

Description
-----------

The ``bson_iter_array()`` function shall retrieve the raw buffer of a sub-array from ``iter``. ``iter`` *MUST* be on an element that is of type BSON_TYPE_ARRAY. This can be verified with :symbol:`bson_iter_type()` or the ``BSON_ITER_HOLDS_ARRAY()`` macro.

