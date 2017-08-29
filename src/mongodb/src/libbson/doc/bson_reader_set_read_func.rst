:man_page: bson_reader_set_read_func

bson_reader_set_read_func()
===========================

Synopsis
--------

.. code-block:: c

  void
  bson_reader_set_read_func (bson_reader_t *reader, bson_reader_read_func_t func);

Parameters
----------

* ``reader``: A :symbol:`bson_reader_t`.
* ``func``: A :symbol:`bson_reader_read_func_t`.

Description
-----------

Sets the function to read more data from the underlying stream in a custom bson_reader_t.

