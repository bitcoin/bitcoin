:man_page: bson_validate

bson_validate()
===============

Synopsis
--------

.. code-block:: c

  bool
  bson_validate (const bson_t *bson, bson_validate_flags_t flags, size_t *offset);

Parameters
----------

* ``bson``: A :symbol:`bson_t`.
* ``flags``: A bitwise-or of all desired :symbol:`bson_validate_flags_t <bson_validate_with_error>`.
* ``offset``: A location for the offset within ``bson`` where the error ocurred.

Description
-----------

Validates a BSON document by walking through the document and inspecting the keys and values for valid content.

You can modify how the validation occurs through the use of the ``flags`` parameter, see :symbol:`bson_validate_with_error()` for details.

Returns
-------

Returns true if ``bson`` is valid; otherwise false and ``offset`` is set to the byte offset where the error was detected.

