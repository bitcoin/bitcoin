:man_page: bson_realloc

bson_realloc()
==============

Synopsis
--------

.. code-block:: c

  void *
  bson_realloc (void *mem, size_t num_bytes);

Parameters
----------

* ``mem``: A memory region.
* ``num_bytes``: A size_t containing the new requested size.

Description
-----------

This is a portable ``realloc()`` wrapper.

In general, this function will return an allocation at least ``sizeof(void*)`` bytes or bigger. If ``num_bytes`` is 0, then the allocation will be freed.

If there was a failure to allocate ``num_bytes`` bytes, the process will be aborted.

.. warning::

  This function will abort on failure to allocate memory.

Returns
-------

A pointer to a memory region which *HAS NOT* been zeroed.

