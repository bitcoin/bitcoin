:man_page: bson_malloc0

bson_malloc0()
==============

Synopsis
--------

.. code-block:: c

  void *
  bson_malloc0 (size_t num_bytes);

Parameters
----------

* ``num_bytes``: A size_t.

Description
-----------

This is a portable ``malloc()`` wrapper that also sets the memory to zero. Similar to ``calloc()``.

In general, this function will return an allocation at least ``sizeof(void*)`` bytes or bigger.

If there was a failure to allocate ``num_bytes`` bytes, the process will be aborted.

.. warning::

  This function will abort on failure to allocate memory.

Returns
-------

A pointer to a memory region which *HAS* been zeroed.

