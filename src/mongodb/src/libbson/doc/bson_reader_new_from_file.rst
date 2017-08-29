:man_page: bson_reader_new_from_file

bson_reader_new_from_file()
===========================

Synopsis
--------

.. code-block:: c

  bson_reader_t *
  bson_reader_new_from_file (const char *path, bson_error_t *error);

Parameters
----------

* ``path``: A filename in the host filename encoding.
* ``error``: A :symbol:`bson_error_t`.

Description
-----------

Creates a new :symbol:`bson_reader_t` using the file denoted by ``filename``.

Errors
------

Errors are propagated via the ``error`` parameter.

Returns
-------

A newly allocated :symbol:`bson_reader_t` on success, otherwise NULL and error is set.

