:man_page: bson_json_reader_new_from_fd

bson_json_reader_new_from_fd()
==============================

Synopsis
--------

.. code-block:: c

  bson_json_reader_t *
  bson_json_reader_new_from_fd (int fd, bool close_on_destroy);

Parameters
----------

* ``fd``: An open file-descriptor.
* ``close_on_destroy``: Whether ``close()`` should be called on ``fd`` when the reader is destroyed.

Description
-----------

Creates a new JSON to BSON converter that will be reading from the file-descriptor ``fd``.

Returns
-------

A newly allocated bson_json_reader_t that should be freed with bson_json_reader_destroy().

