:man_page: bson_strndup

bson_strndup()
==============

Synopsis
--------

.. code-block:: c

  char *
  bson_strndup (const char *str, size_t n_bytes);

Parameters
----------

* ``str``: A string to copy.
* ``n_bytes``: A size_t.

Description
-----------

Allocates a new string and copies up to ``n_bytes`` from ``str`` into it. A trailing ``\0`` is always set.

Returns
-------

A newly allocated string that should be freed with bson_free().

