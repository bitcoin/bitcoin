:man_page: bson_string_truncate

bson_string_truncate()
======================

Synopsis
--------

.. code-block:: c

  void
  bson_string_truncate (bson_string_t *string, uint32_t len);

Parameters
----------

* ``string``: A :symbol:`bson_string_t`.
* ``len``: The new length of the string, excluding the trailing ``\0``.

Description
-----------

Truncates the string so that it is ``len`` bytes in length. This must be smaller or equal to the current length of the string.

A ``\0`` byte will be placed where the end of the string occurrs.

