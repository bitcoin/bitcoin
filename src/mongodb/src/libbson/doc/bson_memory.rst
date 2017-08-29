:man_page: bson_memory

Memory Management
=================

BSON Memory Abstraction.

Description
-----------

Libbson contains a lightweight memory abstraction to make portability to new platforms easier. Additionally, it helps us integrate with interesting higher-level languages. One caveat, however, is that Libbson is not designed to deal with Out of Memory (OOM) situations. Doing so requires extreme dilligence throughout the application stack that has rarely been implemented correctly. This may change in the future. As it stands now, Libbson will ``abort()`` under OOM situations.

To aid in language binding integration, Libbson allows for setting a custom memory allocator via :symbol:`bson_mem_set_vtable()`.  This allocation may be reversed via :symbol:`bson_mem_restore_vtable()`.

.. only:: html

  Functions
  ---------

  .. toctree::
    :titlesonly:
    :maxdepth: 1

    bson_free
    bson_malloc
    bson_malloc0
    bson_mem_restore_vtable
    bson_mem_set_vtable
    bson_realloc
    bson_realloc_ctx
    bson_realloc_func
    bson_zero_free

