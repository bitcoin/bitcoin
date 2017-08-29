:man_page: bson_sized_new

bson_sized_new()
================

Synopsis
--------

.. code-block:: c

  bson_t *
  bson_sized_new (size_t size);

Parameters
----------

* ``size``: The size to pre-allocate for the underlying buffer.

Description
-----------

The :symbol:`bson_sized_new()` function shall create a new :symbol:`bson_t` on the heap with a preallocated buffer. This is useful if you have a good idea of the size of the resulting document.

Returns
-------

A newly allocated :symbol:`bson_t` that should be freed with :symbol:`bson_destroy()`.

.. only:: html

  .. taglist:: See Also:
    :tags: create-bson
