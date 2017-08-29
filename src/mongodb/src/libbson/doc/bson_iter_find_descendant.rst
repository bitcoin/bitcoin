:man_page: bson_iter_find_descendant

bson_iter_find_descendant()
===========================

Synopsis
--------

.. code-block:: c

  bool
  bson_iter_find_descendant (bson_iter_t *iter,
                             const char *dotkey,
                             bson_iter_t *descendant);

Parameters
----------

* ``iter``: A :symbol:`bson_iter_t`.
* ``dotkey``: A dot-notation key like ``"a.b.c.d"``.
* ``descendant``: A :symbol:`bson_iter_t`.

Description
-----------

The :symbol:`bson_iter_find_descendant()` function shall follow standard MongoDB dot notation to recurse into subdocuments. ``descendant`` will be initialized and advanced to the descendant. If false is returned, both ``iter`` and ``descendant`` should be considered invalid.

Returns
-------

true is returned if the requested key was found. If not, false is returned and ``iter`` was exhausted and should now be considered invalid.

