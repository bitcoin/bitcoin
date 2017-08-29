:man_page: bson_iter_find

bson_iter_find()
================

Synopsis
--------

.. code-block:: c

  bool
  bson_iter_find (bson_iter_t *iter, const char *key);

Parameters
----------

* ``iter``: A :symbol:`bson_iter_t`.
* ``key``: A string containing the requested key.

Description
-----------

The ``bson_iter_find()`` function shall advance ``iter`` to the element named ``key`` or exhaust all elements of ``iter``. If ``iter`` is exhausted, false is returned and ``iter`` should be considered invalid.

``key`` is case-sensitive. For a case-folded version, see :symbol:`bson_iter_find_case()`.

Returns
-------

true is returned if the requested key was found. If not, ``iter`` was exhausted and should now be considered invalid.

