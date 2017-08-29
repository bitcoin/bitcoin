:man_page: bson_utf8_next_char

bson_utf8_next_char()
=====================

Synopsis
--------

.. code-block:: c

  const char *
  bson_utf8_next_char (const char *utf8);

Parameters
----------

* ``utf8``: A UTF-8 encoded string.

Description
-----------

Advances within ``utf8`` to the next UTF-8 character, which may be multiple bytes offset from ``utf8``. A pointer to the next character is returned.

It is invalid to call this function on a string whose current character is ``\0``.

Returns
-------

A pointer to the beginning of the next character in the UTF-8 encoded string.

