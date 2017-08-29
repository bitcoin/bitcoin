:man_page: bson_decimal128_t

bson_decimal128_t
=================

BSON Decimal128 Abstraction

Synopsis
--------

.. code-block:: c

  #include <bson.h>

  #define BSON_DECIMAL128_STRING 43
  #define BSON_DECIMAL128_INF "Infinity"
  #define BSON_DECIMAL128_NAN "NaN"

  typedef struct {
  #if BSON_BYTE_ORDER == BSON_LITTLE_ENDIAN
     uint64_t low;
     uint64_t high;
  #elif BSON_BYTE_ORDER == BSON_BIG_ENDIAN
     uint64_t high;
     uint64_t low;
  #endif
  } bson_decimal128_t;

Description
-----------

The :symbol:`bson_decimal128_t` structure
represents the IEEE-754 Decimal128 data type.

.. only:: html

  Functions
  ---------

  .. toctree::
    :titlesonly:
    :maxdepth: 1

    bson_decimal128_from_string
    bson_decimal128_to_string

Example
-------

.. code-block:: c

  #include <bson.h>
  #include <stdio.h>

  int
  main (int argc, char *argv[])
  {
     char string[BSON_DECIMAL128_STRING];
     bson_decimal128_t decimal128;

     bson_decimal128_from_string ("100.00", &decimal128);
     bson_decimal128_to_string (&decimal128, string);
     printf ("Decimal128 value: %s\n", string);

     return 0;
  }

