:man_page: bson_json_reader_new

bson_json_reader_new()
======================

Synopsis
--------

.. code-block:: c

  bson_json_reader_t *
  bson_json_reader_new (void *data,
                        bson_json_reader_cb cb,
                        bson_json_destroy_cb dcb,
                        bool allow_multiple,
                        size_t buf_size);

Parameters
----------

* ``data``: A user-defined pointer.
* ``cb``: A bson_json_reader_cb.
* ``dcb``: A bson_json_destroy_cb.
* ``allow_multiple``: Unused.
* ``buf_size``: A size_t containing the requested internal buffer size.

Description
-----------

Creates a new bson_json_reader_t that can read from an arbitrary data source in a streaming fashion.

The ``allow_multiple`` parameter is unused.

Returns
-------

A newly allocated bson_json_reader_t that should be freed with bson_json_reader_destroy().

