:man_page: bson_reader_read_func_t

bson_reader_read_func_t
=======================

Synopsis
--------

.. code-block:: c

  typedef ssize_t (*bson_reader_read_func_t) (void *handle,
                                              void *buf,
                                              size_t count);

Parameters
----------

* ``handle``: The handle to read from.
* ``buf``: The buffer to read into.
* ``count``: The number of bytes to read.

Description
-----------

A callback function that will be called by :symbol:`bson_reader_t` to read the next chunk of data from the underlying opaque file descriptor.

This function is meant to operate similar to the ``read(2)`` function as part of libc on UNIX-like systems.

Returns
-------

0 for end of stream.

-1 for a failure on read.

A value greater than zero for the number of bytes read into ``buf``.

