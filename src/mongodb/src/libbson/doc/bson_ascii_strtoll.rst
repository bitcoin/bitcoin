:man_page: bson_ascii_strtoll

bson_ascii_strtoll()
====================

Synopsis
--------

.. code-block:: c

  int64_t
  bson_ascii_strtoll (const char *str, char **endptr, int base);

Parameters
----------

* ``str``: The string to convert.
* ``endptr``: Address of the first invalid character of ``str``, or null.
* ``base``: The base to use for the convertion.

Description
-----------

A portable version of ``strtoll()``.


Converts a string to a 64-bit signed integer according to the given ``base``,
which must be 16, 10, or 8. Leading whitespace will be ignored.

If base is 0 is passed in, the base is inferred from the string's leading
characters. Base-16 numbers start with "0x" or "0X", base-8 numbers start with
"0", base-10 numbers start with a digit from 1 to 9.

If ``endptr`` is not NULL, it will be assigned the address of the first invalid
character of ``str``, or its null terminating byte if the entire string was valid.

If an invalid value is encountered, errno will be set to EINVAL and zero will
be returned. If the number is out of range, errno is set to ERANGE and
LLONG_MAX or LLONG_MIN is returned.

Returns
-------

The result of the conversion.
