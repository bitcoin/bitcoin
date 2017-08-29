:man_page: bson_unichar_t

bson_unichar_t
==============

Unicode Character Abstraction

Synopsis
--------

.. code-block:: c

  typedef uint32_t bson_unichar_t;

Description
-----------

:symbol:`bson_unichar_t` provides an abstraction on a single unicode character. It is the 32-bit representation of a character. As UTF-8 can contain multi-byte characters, this should be used when iterating through UTF-8 text.

.. only:: html

  Functions
  ---------

  .. toctree::
    :titlesonly:
    :maxdepth: 1

Example
-------

.. code-block:: c

  static void
  print_each_char (const char *str)
  {
     bson_unichar_t c;

     for (; *str; str = bson_utf8_next_char (str)) {
        c = bson_utf8_get_char (str);
        printf ("The numberic value is %u.\n", (unsigned) c);
     }
  }

