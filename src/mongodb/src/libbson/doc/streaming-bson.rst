:man_page: bson_streaming_bson

Streaming BSON
==============

:symbol:`bson_reader_t` provides a streaming reader which can be initialized with a filedescriptor or memory region. :symbol:`bson_writer_t` provides a streaming writer which can be initialized with a memory region. (Streaming BSON to a file descriptor is not yet supported.)

Reading from a BSON Stream
--------------------------

:symbol:`bson_reader_t` provides a convenient API to read sequential BSON documents from a file-descriptor or memory buffer. The :symbol:`bson_reader_read()` function will read forward in the underlying stream and returna :symbol:`bson_t` that can be inspected and iterated upon.

.. code-block:: c

  #include <stdio.h>
  #include <bson.h>

  int
  main (int argc, char *argv[])
  {
     bson_reader_t *reader;
     const bson_t *doc;
     bson_error_t error;
     bool eof;

     reader = bson_reader_new_from_file ("mycollection.bson", &error);

     if (!reader) {
        fprintf (stderr, "Failed to open file.\n");
        return 1;
     }

     while ((doc = bson_reader_read (reader, &eof))) {
        char *str = bson_as_canonical_extended_json (doc, NULL);
        printf ("%s\n", str);
        bson_free (str);
     }

     if (!eof) {
        fprintf (stderr,
                 "corrupted bson document found at %u\n",
                 (unsigned) bson_reader_tell (reader));
     }

     bson_reader_destroy (reader);

     return 0;
  }

See :symbol:`bson_reader_new_from_fd()`, :symbol:`bson_reader_new_from_file()`, and :symbol:`bson_reader_new_from_data()` for more information.

Writing a sequence of BSON Documents
------------------------------------

:symbol:`bson_writer_t` provides a convenient API to write a sequence of BSON documents to a memory buffer that can grow with ``realloc()``. The :symbol:`bson_writer_begin()` and :symbol:`bson_writer_end()` functions will manage the underlying buffer while building the sequence of documents.

This could also be useful if you want to write to a network packet while serializing the documents from a higher level language, (but do so just after the packets header).

.. code-block:: c

  #include <stdio.h>
  #include <bson.h>
  #include <assert.h>

  int
  main (int argc, char *argv[])
  {
     bson_writer_t *writer;
     bson_t *doc;
     uint8_t *buf = NULL;
     size_t buflen = 0;
     bool r;
     int i;

     writer = bson_writer_new (&buf, &buflen, 0, bson_realloc_ctx, NULL);

     for (i = 0; i < 10000; i++) {
        r = bson_writer_begin (writer, &doc);
        assert (r);

        r = BSON_APPEND_INT32 (doc, "i", i);
        assert (r);

        bson_writer_end (writer);
     }

     bson_free (buf);

     return 0;
  }

See :symbol:`bson_writer_new()` for more information.

