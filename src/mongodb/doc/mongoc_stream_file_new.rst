:man_page: mongoc_stream_file_new

mongoc_stream_file_new()
========================

Synopsis
--------

.. code-block:: c

  mongoc_stream_t *
  mongoc_stream_file_new (int fd);

Parameters
----------

* ``fd``: A UNIX style file-descriptor.

Creates a new :symbol:`mongoc_stream_file_t` using the file-descriptor provided.

Returns
-------

``NULL`` upon failure, otherwise a newly allocated :symbol:`mongoc_stream_file_t` that should be freed with :symbol:`mongoc_stream_destroy()` when no longer in use.

``errno`` is set upon failure.

