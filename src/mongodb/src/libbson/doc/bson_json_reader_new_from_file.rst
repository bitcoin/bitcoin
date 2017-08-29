:man_page: bson_json_reader_new_from_file

bson_json_reader_new_from_file()
================================

Synopsis
--------

.. code-block:: c

  bson_json_reader_t *
  bson_json_reader_new_from_file (const char *filename, bson_error_t *error);

Parameters
----------

* ``filename``: A file-name in the system file-name encoding.
* ``error``: A :symbol:`bson_error_t`.

Description
-----------

Creates a new bson_json_reader_t using the underlying file found at ``filename``.

Errors
------

Errors are propagated via ``error``.

Returns
-------

A newly allocated bson_json_reader_t if successful, otherwise NULL and error is set.

