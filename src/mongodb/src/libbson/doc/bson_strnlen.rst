:man_page: bson_strnlen

bson_strnlen()
==============

Synopsis
--------

.. code-block:: c

  size_t
  bson_strnlen (const char *s, size_t maxlen);

Parameters
----------

* ``s``: A string.
* ``maxlen``: The maximum size of string to check.

Description
-----------

A portable version of ``strnlen()``.

Returns
-------

The length of ``s`` in bytes or ``maxlen`` if no ``\0`` was found.

