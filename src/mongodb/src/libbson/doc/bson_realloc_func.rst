:man_page: bson_realloc_func

bson_realloc_func
=================

Synopsis
--------

.. code-block:: c

  typedef void *(*bson_realloc_func) (void *mem, size_t num_bytes, void *ctx);

Parameters
----------

* ``mem``: A memory region.
* ``num_bytes``: A size_t containing the requested size.
* ``ctx``: A consumer-specific pointer or ``NULL``.

Description
-----------

This is a prototype for pluggable realloc functions used through the Libbson library. If you wish to use a custom allocator this is one way to do it. Additionally, :symbol:`bson_realloc_ctx()` is a default implementation of this prototype.

