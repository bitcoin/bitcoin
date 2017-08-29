:man_page: bson_error_t

bson_error_t
============

BSON Error Encapsulation

Synopsis
--------

.. code-block:: c

  #include <bson.h>

  typedef struct {
     uint32_t domain;
     uint32_t code;
     char message[504];
  } bson_error_t;

Description
-----------

The :symbol:`bson_error_t` structure is used as an out-parameter to pass error information to the caller. It should be stack-allocated and does not requiring freeing.

See :doc:`Handling Errors <errors>`.

.. only:: html

  Functions
  ---------

  .. toctree::
    :titlesonly:
    :maxdepth: 1

    bson_set_error
    bson_strerror_r

Example
-------

.. code-block:: c

  bson_reader_t *reader;
  bson_error_t error;

  reader = bson_reader_new_from_file ("dump.bson", &error);
  if (!reader) {
     fprintf (
        stderr, "ERROR: %d.%d: %s\n", error.domain, error.code, error.message);
  }

