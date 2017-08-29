:man_page: bson_reader_read

bson_reader_read()
==================

Synopsis
--------

.. code-block:: c

  const bson_t *
  bson_reader_read (bson_reader_t *reader, bool *reached_eof);

Parameters
----------

* ``reader``: A :symbol:`bson_reader_t`.
* ``reached_eof``: A UNKNOWN.

Description
-----------

The :symbol:`bson_reader_read()` function shall read the next document from the underlying file-descriptor or buffer.

If there are no further documents or a failure was detected, then NULL is returned.

If we reached the end of the sequence, ``reached_eof`` is set to true.

To detect an error, check for NULL and ``reached_of`` is false.

Returns
-------

A :symbol:`bson_t` that should not be modified or freed.

Example
-------

.. code-block:: c

  const bson_t *doc;
  bool reached_eof = false;

  while ((doc = bson_reader_read (reader, &reached_eof))) {
     /* do something */
  }

  if (!reached_eof) {
     fprintf (stderr, "Failed to read all documents.\n");
  }

