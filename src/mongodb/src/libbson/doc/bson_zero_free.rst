:man_page: bson_zero_free

bson_zero_free()
================

Synopsis
--------

.. code-block:: c

  void
  bson_zero_free (void *mem, size_t size);

Parameters
----------

* ``mem``: A memory region.
* ``size``: The size of ``mem``.

Description
-----------

This function behaves like :symbol:`bson_free()` except that it zeroes the memory first. This can be useful if you are storing passwords or other similarly important data. Note that if it truly is important, you probably want a page protected with ``mlock()`` as well to prevent it swapping to disk.

