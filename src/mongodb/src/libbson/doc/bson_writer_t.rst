:man_page: bson_writer_t

bson_writer_t
=============

Bulk BSON serialization Abstraction

Synopsis
--------

.. code-block:: c

  #include <bson.h>

  typedef struct _bson_writer_t bson_writer_t;

  bson_writer_t *
  bson_writer_new (uint8_t **buf,
                   size_t *buflen,
                   size_t offset,
                   bson_realloc_func realloc_func,
                   void *realloc_func_ctx);
  void
  bson_writer_destroy (bson_writer_t *writer);

Description
-----------

The :symbol:`bson_writer_t` API provides an abstraction for serializing many BSON documents to a single memory region. The memory region may be dynamically allocated and re-allocated as more memory is demanded. This can be useful when building network packets from a high-level language. For example, you can serialize a Python Dictionary directly to a single buffer destined for a TCP packet.

.. only:: html

  Functions
  ---------

  .. toctree::
    :titlesonly:
    :maxdepth: 1

    bson_writer_begin
    bson_writer_destroy
    bson_writer_end
    bson_writer_get_length
    bson_writer_new
    bson_writer_rollback

Example
-------

.. code-block:: c

  #include <bson.h>

  int
  main (int argc, char *argv[])
  {
     bson_writer_t *writer;
     uint8_t *buf = NULL;
     size_t buflen = 0;
     bson_t *doc;

     writer = bson_writer_new (&buf, &buflen, 0, bson_realloc_ctx, NULL);

     for (i = 0; i < 1000; i++) {
        bson_writer_begin (writer, &doc);
        BSON_APPEND_INT32 (&doc, "i", i);
        bson_writer_end (writer);
     }

     bson_writer_destroy (writer);

     bson_free (buf);

     return 0;
  }

