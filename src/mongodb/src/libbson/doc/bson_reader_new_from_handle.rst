:man_page: bson_reader_new_from_handle

bson_reader_new_from_handle()
=============================

Synopsis
--------

.. code-block:: c

  bson_reader_t *
  bson_reader_new_from_handle (void *handle,
                               bson_reader_read_func_t rf,
                               bson_reader_destroy_func_t df);

Parameters
----------

* ``handle``: A user-provided pointer or NULL.
* ``rf``: A :symbol:`bson_reader_read_func_t`.
* ``df``: A :symbol:`bson_reader_destroy_func_t`.

Description
-----------

This function allows for a pluggable data stream for the reader. This can be used to read from sockets, files, memory, or other arbitrary sources.

Returns
-------

A newly allocated bson_reader_t if successful; otherwise NULL.

