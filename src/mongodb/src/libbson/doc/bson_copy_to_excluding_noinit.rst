:man_page: bson_copy_to_excluding_noinit

bson_copy_to_excluding_noinit()
===============================

Synopsis
--------

.. code-block:: c

  void
  bson_copy_to_excluding_noinit (const bson_t *src,
                                 bson_t *dst,
                                 const char *first_exclude,
                                 ...) BSON_GNUC_NULL_TERMINATED;

Parameters
----------

* ``src``: A :symbol:`bson_t`.
* ``dst``: A :symbol:`bson_t`.
* ``first_exclude``: The first field name to exclude.

Description
-----------

The :symbol:`bson_copy_to_excluding_noinit()` function shall copy all fields
from ``src`` to ``dst`` except those speified by the variadic, NULL terminated
list of keys starting from ``first_exclude``.
Works the same way as :symbol:`bson_copy_to_excluding`, except does **not** call
:symbol:`bson_init` on ``dst``.
This function should be preferred in new code over :symbol:`bson_copy_to_excluding`.

.. warning::

  This is generally not needed except in very special situations.

Example
-------

.. code-block:: c

  #include <bson.h>

  int main ()
  {
     bson_t bson;
     bson_t bson2;
     char *str;

     bson_init (&bson);
     bson_append_int32 (&bson, "a", 1, 1);
     bson_append_int32 (&bson, "b", 1, 2);
     bson_append_int32 (&bson, "c", 1, 2);

     bson_init (&bson2);
     bson_copy_to_excluding_noinit (&bson, &bson2, "b", NULL);

     str = bson_as_json (&bson2, NULL);
     /* Prints
      * { "a" : 1, "c" : 2 }
      */
     printf ("%s\n", str);
     bson_free (str);
 
     bson_destroy (&bson);
     bson_destroy (&bson2);
  }

