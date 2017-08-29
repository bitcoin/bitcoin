:man_page: bson_utf8

UTF-8
=====

Encoding
--------

Libbson expects that you are always working with UTF-8 encoded text. Anything else is **invalid API use**.

If you should need to walk through UTF-8 sequences, you can use the various UTF-8 helper functions distributed with Libbson.

Validating a UTF-8 Sequence
---------------------------

To validate the string contained in ``my_string``, use the following. You may pass ``-1`` for the string length if you know the string is NULL-terminated.

.. code-block:: c

  if (!bson_utf8_validate (my_string, -1, false)) {
     printf ("Validation failed.\n");
  }

If ``my_string`` has NULL bytes within the string, you must provide the string length. Use the following format. Notice the ``true`` at the end indicationg ``\0`` is allowed.

.. code-block:: c

  if (!bson_utf8_validate (my_string, my_string_len, true)) {
     printf ("Validation failed.\n");
  }

For more information see the API reference for :symbol:`bson_utf8_validate()`.

