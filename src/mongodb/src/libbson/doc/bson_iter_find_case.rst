:man_page: bson_iter_find_case

bson_iter_find_case()
=====================

Synopsis
--------

.. code-block:: c

  bool
  bson_iter_find_case (bson_iter_t *iter, const char *key);

Parameters
----------

* ``iter``: A :symbol:`bson_iter_t`.
* ``key``: An ASCII string containing the field to locate.

Description
-----------

Advances ``iter`` until it is observing an element matching the name of ``key`` or exhausting all elements.

``key`` is not case-sensitive. The keys will be case-folded to determine a match using the current locale.

Returns
-------

true if ``key`` was found.

