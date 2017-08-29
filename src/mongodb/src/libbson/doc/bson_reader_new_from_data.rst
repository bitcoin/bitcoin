:man_page: bson_reader_new_from_data

bson_reader_new_from_data()
===========================

Synopsis
--------

.. code-block:: c

  bson_reader_t *
  bson_reader_new_from_data (const uint8_t *data, size_t length);

Parameters
----------

* ``data``: A uint8_t.
* ``length``: A size_t.

Description
-----------

The :symbol:`bson_reader_new_from_data()` function shall create a new :symbol:`bson_reader_t` using the buffer supplied. ``data`` is not copied and *MUST* be valid for the lifetime of the resulting :symbol:`bson_reader_t`.

Returns
-------

A newly allocated :symbol:`bson_reader_t`.

