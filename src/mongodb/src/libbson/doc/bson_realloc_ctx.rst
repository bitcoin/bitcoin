:man_page: bson_realloc_ctx

bson_realloc_ctx()
==================

Synopsis
--------

.. code-block:: c

  void *
  bson_realloc_ctx (void *mem, size_t num_bytes, void *ctx);

Parameters
----------

* ``mem``: A memory region.
* ``num_bytes``: A size_t containing the requested size.
* ``ctx``: A consumer-specific pointer or ``NULL``.

Description
-----------

This function is identical to :symbol:`bson_realloc()` except it takes a context parameter. This is useful when working with pooled or specific memory allocators.

