:man_page: bson_context_get_default

bson_context_get_default()
==========================

Synopsis
--------

.. code-block:: c

  bson_context_t *
  bson_context_get_default (void) BSON_GNUC_CONST;

Returns
-------

The ``bson_context_get_default()`` function shall return the default, thread-safe, :symbol:`bson_context_t` for the process.

