:man_page: mongoc_stream_file_new_for_path

mongoc_stream_file_new_for_path()
=================================

Synopsis
--------

.. code-block:: c

  mongoc_stream_t *
  mongoc_stream_file_new_for_path (const char *path, int flags, int mode);

Parameters
----------

* ``path``: The path of the target file.
* ``flags``: Flags to be passed to ``open()``.
* ``mode``: An optional mode to be passed to ``open()`` when creating a file.

This function shall create a new :symbol:`mongoc_stream_file_t` after opening the underlying file with ``open()`` or the platform equivalent.

Returns
-------

``NULL`` on failure, otherwise a newly allocated :symbol:`mongoc_stream_file_t` that should be freed with :symbol:`mongoc_stream_destroy()` when no longer in use.

``errno`` is set upon failure.

