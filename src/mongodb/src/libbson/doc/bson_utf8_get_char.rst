:man_page: bson_utf8_get_char

bson_utf8_get_char()
====================

Synopsis
--------

.. code-block:: c

  bson_unichar_t
  bson_utf8_get_char (const char *utf8);

Parameters
----------

* ``utf8``: A UTF-8 encoded string.

Description
-----------

Converts the current character in a UTF-8 sequence to a bson_unichar_t, the 32-bit representation of the multi-byte character.

Returns
-------

A bson_unichar_t.

