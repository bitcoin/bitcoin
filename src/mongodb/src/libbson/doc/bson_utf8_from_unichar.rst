:man_page: bson_utf8_from_unichar

bson_utf8_from_unichar()
========================

Synopsis
--------

.. code-block:: c

  void
  bson_utf8_from_unichar (bson_unichar_t unichar, char utf8[6], uint32_t *len);

Parameters
----------

* ``unichar``: A :symbol:`bson_unichar_t`.
* ``utf8``: A location for the utf8 sequence.
* ``len``: A location for the number of bytes in the resulting utf8 sequence.

Description
-----------

Converts a single unicode character to a multi-byte UTF-8 sequence. The result is stored in ``utf8`` and the number of bytes used in ``len``.

