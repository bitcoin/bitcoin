:man_page: bson_utf8_validate

bson_utf8_validate()
====================

Synopsis
--------

.. code-block:: c

  bool
  bson_utf8_validate (const char *utf8, size_t utf8_len, bool allow_null);

Parameters
----------

* ``utf8``: A string to verify.
* ``utf8_len``: The length of ``utf8`` in bytes.
* ``allow_null``: A bool indicating that embedded ``\0`` bytes are allowed.

Description
-----------

Validates that the content within ``utf8`` is valid UTF-8. If ``allow_null`` is ``true``, then embedded NULL bytes are allowed (``\0``).

Returns
-------

true if ``utf8`` contains valid UTF-8.

