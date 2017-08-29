:man_page: bson_reader_reset

bson_reader_reset()
===================

Synopsis
--------

.. code-block:: c

  void
  bson_reader_reset (bson_reader_t *reader);

Parameters
----------

* ``reader``: A :symbol:`bson_reader_t`.

Description
-----------

Seeks to the beginning of the underlying buffer. Valid only for a reader created from a buffer with :symbol:`bson_reader_new_from_data`, not one created from a file, file descriptor, or handle.

