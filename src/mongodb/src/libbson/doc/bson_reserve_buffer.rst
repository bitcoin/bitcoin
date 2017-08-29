:man_page: bson_reserve_buffer

bson_reserve_buffer()
=====================

Synopsis
--------

.. code-block:: c

  uint8_t *
  bson_reserve_buffer (bson_t *bson, uint32_t size);

Parameters
----------

* ``bson``: An initialized :symbol:`bson_t`.
* ``size``: The length in bytes of the new buffer.

Description
-----------

Grow the internal buffer of ``bson`` to ``size`` and set the document length to ``size``. Useful for eliminating copies when reading BSON bytes from a stream.

First, initialize ``bson`` with :symbol:`bson_init` or :symbol:`bson_new`, then call this function. After it returns, the length of ``bson`` is set to ``size`` but its contents are uninitialized memory: you must fill the contents with a BSON document of the correct length before any other operations.

The document must be freed with :symbol:`bson_destroy`.

Returns
-------

A pointer to the internal buffer, which is at least ``size`` bytes, or NULL if the space could not be allocated.

Example
-------

Use ``bson_reserve_buffer`` to write a function that takes a :symbol:`bson_t` pointer and reads a file into it directly:

.. code-block:: c

  #include <stdio.h>
  #include <bson.h>

  bool
  read_into (bson_t *bson, FILE *fp)
  {
     uint8_t *buffer;
     long size;

     if (fseek (fp, 0L, SEEK_END) < 0) {
        perror ("Couldn't get file size");
        return 1;
     }

     size = ftell (fp);
     if (size == EOF) {
        perror ("Couldn't get file size");
        return 1;
     }

     if (size > INT32_MAX) {
        fprintf (stderr, "File too large\n");
        return 1;
     }

     /* reserve buffer space - bson is temporarily invalid */
     buffer = bson_reserve_buffer (bson, (uint32_t) size);
     if (!buffer) {
        fprintf (stderr, "Couldn't reserve %ld bytes", size);
        return false;
     }

     /* read file directly into the buffer */
     rewind (fp);
     if (fread ((void *) buffer, 1, (size_t) size, fp) < (size_t) size) {
        perror ("Couldn't read file");
        return false;
     }

     return true;
  }

  int
  main ()
  {
     FILE *fp;
     char *json;

     /* stack-allocated, initialized bson_t */
     bson_t bson = BSON_INITIALIZER;

     if (!(fp = fopen ("document.bson", "rb"))) {
        perror ("Couldn't read file");
        return 1;
     }

     read_into (&bson, fp);
     fclose (fp);

     json = bson_as_canonical_extended_json (&bson, NULL);
     printf ("%s\n", json);

     bson_free (json);
     bson_destroy (&bson);

     return 0;
  }

.. only:: html

  .. taglist:: See Also:
    :tags: create-bson
