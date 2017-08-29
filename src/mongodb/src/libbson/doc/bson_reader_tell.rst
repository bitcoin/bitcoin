:man_page: bson_reader_tell

bson_reader_tell()
==================

Synopsis
--------

.. code-block:: c

  off_t
  bson_reader_tell (bson_reader_t *reader);

Parameters
----------

* ``reader``: A :symbol:`bson_reader_t`.

Description
-----------

Tells the current position within the underlying stream.

Returns
-------

-1 on failure, otherwise the offset within the underlying stream.

