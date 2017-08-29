:man_page: bson_context_new

bson_context_new()
==================

Synopsis
--------

.. code-block:: c

  bson_context_t *
  bson_context_new (bson_context_flags_t flags);

Parameters
----------

* ``flags``: A :symbol:`bson_context_flags_t <bson_context_t>`.

Description
-----------

Creates a new :symbol:`bson_context_t`. This is rarely needed as :symbol:`bson_context_get_default()` serves most use-cases.

Returns
-------

A newly allocated :symbol:`bson_context_t` that should be freed with :symbol:`bson_context_destroy`.

