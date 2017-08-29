:man_page: bson_context_destroy

bson_context_destroy()
======================

Synopsis
--------

.. code-block:: c

  void
  bson_context_destroy (bson_context_t *context);

Parameters
----------

* ``context``: A :symbol:`bson_context_t`.

Description
-----------

The ``bson_context_destroy()`` function shall release all resources associated with ``context``.

This should be called when you are no longer using a :symbol:`bson_context_t` that you have allocated with :symbol:`bson_context_new()`.

