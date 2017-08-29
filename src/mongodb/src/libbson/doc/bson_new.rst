:man_page: bson_new

bson_new()
==========

Synopsis
--------

.. code-block:: c

  bson_t *
  bson_new (void);

Description
-----------

The :symbol:`bson_new()` function shall create a new :symbol:`bson_t` structure on the heap. It should be freed with :symbol:`bson_destroy()` when it is no longer in use.

Returns
-------

A newly allocated :symbol:`bson_t` that should be freed with :symbol:`bson_destroy()`.

.. only:: html

  .. taglist:: See Also:
    :tags: create-bson
