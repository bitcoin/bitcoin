:man_page: bson_validate_with_error

bson_validate_with_error()
==========================

Synopsis
--------

.. code-block:: c

  typedef enum {
     BSON_VALIDATE_NONE = 0,
     BSON_VALIDATE_UTF8 = (1 << 0),
     BSON_VALIDATE_DOLLAR_KEYS = (1 << 1),
     BSON_VALIDATE_DOT_KEYS = (1 << 2),
     BSON_VALIDATE_UTF8_ALLOW_NULL = (1 << 3),
     BSON_VALIDATE_EMPTY_KEYS = (1 << 4),
  } bson_validate_flags_t;

  bool
  bson_validate_with_error (const bson_t *bson,
                            bson_validate_flags_t flags,
                            bson_error_t *error);

Parameters
----------

* ``bson``: A :symbol:`bson_t`.
* ``flags``: A bitwise-or of all desired validation flags.
* ``error``: Optional :symbol:`bson_error_t`.

Description
-----------

Validates a BSON document by walking through the document and inspecting the keys and values for valid content.

You can modify how the validation occurs through the use of the ``flags`` parameter. A description of their effect is below.

* ``BSON_VALIDATE_NONE`` Basic validation of BSON length and structure.
* ``BSON_VALIDATE_UTF8`` All keys and string values are checked for invalid UTF-8.
* ``BSON_VALIDATE_UTF8_ALLOW_NULL`` String values are allowed to have embedded NULL bytes.
* ``BSON_VALIDATE_DOLLAR_KEYS`` Prohibit keys that start with ``$`` outside of a "DBRef" subdocument.
* ``BSON_VALIDATE_DOT_KEYS`` Prohibit keys that contain ``.`` anywhere in the string.
* ``BSON_VALIDATE_EMPTY_KEYS`` Prohibit zero-length keys.

See also :symbol:`bson_validate()`.

Returns
-------

Returns true if ``bson`` is valid; otherwise false and ``error`` is filled out.

The :symbol:`bson_error_t` domain is set to ``BSON_ERROR_INVALID``. Its code is set to one of the ``bson_validate_flags_t`` flags indicating which validation failed; for example, if a key contains invalid UTF-8, then the code is set to ``BSON_VALIDATE_UTF8``, but if the basic structure of the BSON document is corrupt, the code is set to ``BSON_VALIDATE_NONE``. The error message is filled out, and gives more detail if possible.
