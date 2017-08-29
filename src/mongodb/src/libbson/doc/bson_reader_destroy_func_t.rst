:man_page: bson_reader_destroy_func_t

bson_reader_destroy_func_t
==========================

Synopsis
--------

.. code-block:: c

  typedef void (*bson_reader_destroy_func_t) (void *handle);

Parameters
----------

* ``handle``: The opaque handle provided to :symbol:`bson_reader_new_from_handle`.

Description
-----------

An optional callback function that will be called when a :symbol:`bson_reader_t` created with :symbol:`bson_reader_new_from_handle` is destroyed with :symbol:`bson_reader_destroy()`.

The handle used when creating the reader is passed to this callback.

