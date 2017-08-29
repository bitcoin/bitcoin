:man_page: mongoc_gridfs_file_seek

mongoc_gridfs_file_seek()
=========================

Synopsis
--------

.. code-block:: c

  int
  mongoc_gridfs_file_seek (mongoc_gridfs_file_t *file, int64_t delta, int whence);

Parameters
----------

* ``file``: A :symbol:`mongoc_gridfs_file_t`.
* ``delta``: The amount to move the file position. May be positive or negative.
* ``whence``: One of SEEK_SET, SEEK_CUR or SEEK_END.

Description
-----------

Adjust the file position pointer in the given file by ``delta``, starting from the position ``whence``. The ``whence`` argument is interpreted as in ``fseek(2)``:

============  =====================================================
``SEEK_SET``  Set the position relative to the start of the file.  
``SEEK_CUR``  Move ``delta`` relative to the current file position.
``SEEK_END``  Move ``delta`` relative to the end of the file.      
============  =====================================================

On success, the file's underlying position pointer is set appropriately. On failure, the file position is NOT changed and errno is set to indicate the error.

Errors
------

==========  ========================================================
``EINVAL``  ``whence`` is not one of SEEK_SET, SEEK_CUR or SEEK_END.
``EINVAL``  The resulting file position would be negative.          
==========  ========================================================

Returns
-------

Returns 0 if successful; otherwise -1 and errno is set.

