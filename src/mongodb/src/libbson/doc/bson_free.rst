:man_page: bson_free

bson_free()
===========

Synopsis
--------

.. code-block:: c

  void
  bson_free (void *mem);

Parameters
----------

* ``mem``: A memory region.

Description
-----------

This function shall free the memory supplied by ``mem``. This should be used by functions that require you free the result with ``bson_free()``.

