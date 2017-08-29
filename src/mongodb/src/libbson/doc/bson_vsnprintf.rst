:man_page: bson_vsnprintf

bson_vsnprintf()
================

Synopsis
--------

.. code-block:: c

  int
  bson_vsnprintf (char *str, size_t size, const char *format, va_list ap)
     BSON_GNUC_PRINTF (3, 0);

Parameters
----------

* ``str``: A location for the resulting string.
* ``size``: The size of str in bytes.
* ``format``: A printf style format string.
* ``ap``: A va_list of parameters for the format.

Description
-----------

Like bson_snprintf() but allows for variadic parameters.

Returns
-------

The number of bytes written to ``str`` excluding the ``\0`` byte.

