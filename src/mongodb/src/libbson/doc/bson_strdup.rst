:man_page: bson_strdup

bson_strdup()
=============

Synopsis
--------

.. code-block:: c

  char *
  bson_strdup (const char *str);

Parameters
----------

* ``str``: A string.

Description
-----------

Copies ``str`` into a new string. If ``str`` is NULL, then NULL is returned.

Returns
-------

A newly allocated string that should be freed with bson_free().

