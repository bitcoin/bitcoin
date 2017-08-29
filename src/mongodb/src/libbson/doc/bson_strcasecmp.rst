:man_page: bson_strcasecmp

bson_strcasecmp()
=================

Synopsis
--------

.. code-block:: c

  int
  bson_strcasecmp (const char *s1, const char *s2);

Parameters
----------

* ``s1``: A string.
* ``s2``: A string.

Description
-----------

A portable version of ``strcasecmp()``.

Returns
-------

Returns a negative integer if s1 sorts lexicographically before s2, a positive
integer if it sorts after, or 0 if they are equivalent, after translating both
strings to lower case.
