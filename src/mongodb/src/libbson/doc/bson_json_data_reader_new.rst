:man_page: bson_json_data_reader_new

bson_json_data_reader_new()
===========================

Synopsis
--------

.. code-block:: c

  bson_json_reader_t *
  bson_json_data_reader_new (bool allow_multiple, size_t size);

Parameters
----------

* ``allow_multiple``: Unused.
* ``size``: A requested buffer size.

Description
-----------

Creates a new streaming JSON reader that will convert JSON documents to BSON.

The ``allow_multiple`` parameter is unused.

Returns
-------

A newly allocated bson_json_reader_t that should be freed with bson_json_reader_destroy().

