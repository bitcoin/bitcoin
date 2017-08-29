:man_page: bson_string_append

bson_string_append()
====================

Synopsis
--------

.. code-block:: c

  void
  bson_string_append (bson_string_t *string, const char *str);

Parameters
----------

* ``string``: A :symbol:`bson_string_t`.
* ``str``: A string.

Description
-----------

Appends the ASCII or UTF-8 encoded string ``str`` to ``string``. This is not suitable for embedding NULLs in strings.

