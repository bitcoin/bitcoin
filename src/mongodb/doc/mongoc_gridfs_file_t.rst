:man_page: mongoc_gridfs_file_t

mongoc_gridfs_file_t
====================

Synopsis
--------

.. code-block:: c

  typedef struct _mongoc_gridfs_file_t mongoc_gridfs_file_t;

Description
-----------

This structure provides a MongoDB GridFS file abstraction. It provides several APIs.

* readv, writev, seek, and tell.
* General file metadata such as filename and length.
* GridFS metadata such as md5, filename, content_type, aliases, metadata, chunk_size, and upload_date.

Thread Safety
-------------

This structure is NOT thread-safe and should only be used from one thread at a time.

.. only:: html

  Functions
  ---------

  .. toctree::
    :titlesonly:
    :maxdepth: 1

    mongoc_gridfs_file_destroy
    mongoc_gridfs_file_error
    mongoc_gridfs_file_get_aliases
    mongoc_gridfs_file_get_chunk_size
    mongoc_gridfs_file_get_content_type
    mongoc_gridfs_file_get_filename
    mongoc_gridfs_file_get_id
    mongoc_gridfs_file_get_length
    mongoc_gridfs_file_get_md5
    mongoc_gridfs_file_get_metadata
    mongoc_gridfs_file_get_upload_date
    mongoc_gridfs_file_readv
    mongoc_gridfs_file_remove
    mongoc_gridfs_file_save
    mongoc_gridfs_file_seek
    mongoc_gridfs_file_set_aliases
    mongoc_gridfs_file_set_content_type
    mongoc_gridfs_file_set_filename
    mongoc_gridfs_file_set_id
    mongoc_gridfs_file_set_md5
    mongoc_gridfs_file_set_metadata
    mongoc_gridfs_file_tell
    mongoc_gridfs_file_writev

Related
-------

* :symbol:`mongoc_client_t`
* :symbol:`mongoc_gridfs_t`
* :symbol:`mongoc_gridfs_file_list_t`
* :symbol:`mongoc_gridfs_file_opt_t`

