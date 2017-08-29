:man_page: bson_snprintf

bson_snprintf()
===============

Synopsis
--------

.. code-block:: c

  int
  bson_snprintf (char *str, size_t size, const char *format, ...)
     BSON_GNUC_PRINTF (3, 4);

Parameters
----------

* ``str``: The destination buffer.
* ``size``: The size of ``str``.
* ``format``: A printf style format string.

Description
-----------

This is a portable wrapper around ``snprintf()``. It also enforces a trailing ``\0`` in the resulting string.

Returns
-------

The number of bytes written to ``str``.

