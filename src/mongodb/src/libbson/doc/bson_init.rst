:man_page: bson_init

bson_init()
===========

Synopsis
--------

.. code-block:: c

  void
  bson_init (bson_t *b);

Parameters
----------

* ``b``: A :symbol:`bson_t`.

Description
-----------

The :symbol:`bson_init()` function shall initialize a :symbol:`bson_t` that is placed on the stack. This is equivalent to initializing a :symbol:`bson_t` to ``BSON_INITIALIZER``.

.. only:: html

  .. taglist:: See Also:
    :tags: create-bson
