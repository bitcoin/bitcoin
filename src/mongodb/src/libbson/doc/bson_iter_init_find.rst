:man_page: bson_iter_init_find

bson_iter_init_find()
=====================

Synopsis
--------

.. code-block:: c

  bool
  bson_iter_init_find (bson_iter_t *iter, const bson_t *bson, const char *key);

Parameters
----------

* ``iter``: A :symbol:`bson_iter_t`.
* ``bson``: A :symbol:`bson_t`.
* ``key``: A key to locate after initializing the iter.

Description
-----------

This function is identical to ``(bson_iter_init() && bson_iter_find())``.

.. only:: html

  .. taglist:: See Also:
    :tags: iter-init
