:man_page: bson_utf8_escape_for_json

bson_utf8_escape_for_json()
===========================

Synopsis
--------

.. code-block:: c

  char *
  bson_utf8_escape_for_json (const char *utf8, ssize_t utf8_len);

Parameters
----------

* ``utf8``: A UTF-8 encoded string.
* ``utf8_len``: The length of ``utf8`` in bytes or -1 if it is NULL terminated.

Description
-----------

Allocates a new string matching ``utf8`` except that special
characters in JSON are escaped. The resulting string is also
UTF-8 encoded.

Both " and \\ characters will be backslash-escaped. If a NUL
byte is found before ``utf8_len`` bytes, it is converted to
"\\u0000". Other non-ASCII characters in the input are preserved.

Returns
-------

A newly allocated string that should be freed with :symbol:`bson_free()`.
