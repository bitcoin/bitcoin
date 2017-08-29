:man_page: bson_iter_binary

bson_iter_binary()
==================

Synopsis
--------

.. code-block:: c

  #define BSON_ITER_HOLDS_BINARY(iter) \
     (bson_iter_type ((iter)) == BSON_TYPE_BINARY)

  void
  bson_iter_binary (const bson_iter_t *iter,
                    bson_subtype_t *subtype,
                    uint32_t *binary_len,
                    const uint8_t **binary);

Parameters
----------

* ``iter``: A :symbol:`bson_iter_t`.
* ``subtype``: A location for a :symbol:`bson_subtype_t` or NULL.
* ``binary_len``: A location for the length of ``binary``.
* ``binary``: A location for a pointer to the immutable buffer.

Description
-----------

This function shall return the binary data of a BSON_TYPE_BINARY element. It is a programming error to call this function on a field that is not of type BSON_TYPE_BINARY. You can check this with the BSON_ITER_HOLDS_BINARY() macro or :symbol:`bson_iter_type()`.

The buffer that ``binary`` points to is only valid until the iterator's :symbol:`bson_t` is modified or freed.

