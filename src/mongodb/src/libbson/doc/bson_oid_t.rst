:man_page: bson_oid_t

bson_oid_t
==========

BSON ObjectID Abstraction

Synopsis
--------

.. code-block:: c

  #include <bson.h>

  typedef struct {
     uint8_t bytes[12];
  } bson_oid_t;

Description
-----------

The :symbol:`bson_oid_t` structure contains the 12-byte ObjectId notation defined by the `BSON ObjectID specification <http://docs.mongodb.org/manual/reference/object-id/>`_.

ObjectId is a 12-byte BSON type, constructed using:

* a 4-byte value representing the seconds since the Unix epoch (in Big Endian)
* a 3-byte machine identifier
* a 2-byte process id (Big Endian), and
* a 3-byte counter (Big Endian), starting with a random value.

String Conversion
-----------------

You can convert an Object ID to a string using :symbol:`bson_oid_to_string()` and back with :symbol:`bson_oid_init_from_string()`.

Hashing
-------

A :symbol:`bson_oid_t` can be used in hashtables using the function :symbol:`bson_oid_hash()` and :symbol:`bson_oid_equal()`.

Comparing
---------

A :symbol:`bson_oid_t` can be compared to another using :symbol:`bson_oid_compare()` for ``qsort()`` style comparing and :symbol:`bson_oid_equal()` for direct equality.

Validating
----------

You can validate that a string containing a hex-encoded ObjectID is valid using the function :symbol:`bson_oid_is_valid()`.

.. only:: html

  Functions
  ---------

  .. toctree::
    :titlesonly:
    :maxdepth: 1

    bson_oid_compare
    bson_oid_copy
    bson_oid_equal
    bson_oid_get_time_t
    bson_oid_hash
    bson_oid_init
    bson_oid_init_from_data
    bson_oid_init_from_string
    bson_oid_init_sequence
    bson_oid_is_valid
    bson_oid_to_string

Example
-------

.. code-block:: c

  #include <bson.h>
  #include <stdio.h>

  int
  main (int argc, char *argv[])
  {
     bson_oid_t oid;
     char str[25];

     bson_oid_init (&oid, NULL);
     bson_oid_to_string (&oid, str);
     printf ("%s\n", str);

     if (bson_oid_is_valid (str, sizeof str)) {
        bson_oid_init_from_string (&oid, str);
     }

     printf ("The UNIX time was: %u\n", (unsigned) bson_oid_get_time_t (&oid));

     return 0;
  }

