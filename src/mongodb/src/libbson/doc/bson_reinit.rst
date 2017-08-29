:man_page: bson_reinit

bson_reinit()
=============

Synopsis
--------

.. code-block:: c

  void
  bson_reinit (bson_t *b);

Parameters
----------

* ``b``: A :symbol:`bson_t`.

Description
-----------

The :symbol:`bson_reinit()` function shall be equivalent to calling :symbol:`bson_destroy()` and :symbol:`bson_init()`.

However, if the :symbol:`bson_t` structure contains a malloc()'d buffer, it may be reused. To be certain that any buffer is freed, always call :symbol:`bson_destroy` on any :symbol:`bson_t` structure, whether initialized or reinitialized, after its final use.

.. only:: html

  .. taglist:: See Also:
    :tags: create-bson
