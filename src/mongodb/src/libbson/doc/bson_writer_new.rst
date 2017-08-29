:man_page: bson_writer_new

bson_writer_new()
=================

Synopsis
--------

.. code-block:: c

  bson_writer_t *
  bson_writer_new (uint8_t **buf,
                   size_t *buflen,
                   size_t offset,
                   bson_realloc_func realloc_func,
                   void *realloc_func_ctx);

Parameters
----------

* ``buf``: A uint8_t.
* ``buflen``: A size_t.
* ``offset``: A size_t.
* ``realloc_func``: A bson_realloc_func.
* ``realloc_func_ctx``: A bson_realloc_func.

Description
-----------

Creates a new instance of :symbol:`bson_writer_t` using the ``buffer``, ``length``, ``offset``, and _realloc()_ function supplied.

The caller is expected to clean up the structure when finished using :symbol:`bson_writer_destroy()`.

Returns
-------

A newly allocated bson_writer_t.

