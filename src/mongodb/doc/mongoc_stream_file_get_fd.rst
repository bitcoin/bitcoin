:man_page: mongoc_stream_file_get_fd

mongoc_stream_file_get_fd()
===========================

Synopsis
--------

.. code-block:: c

  int
  mongoc_stream_file_get_fd (mongoc_stream_file_t *stream);

Parameters
----------

* ``stream``: A :symbol:`mongoc_stream_file_t`.

This function shall return the underlying file-descriptor of a :symbol:`mongoc_stream_file_t`.

.. warning::

  Performing operations on the underlying file-descriptor may not be safe if used in conjunction with buffering. Avoid reading or writing from this file-descriptor.

Returns
-------

A file-descriptor that should not be modified by the caller.

