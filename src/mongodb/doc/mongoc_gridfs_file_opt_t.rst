:man_page: mongoc_gridfs_file_opt_t

mongoc_gridfs_file_opt_t
========================

Synopsis
--------

.. code-block:: c

  typedef struct {
     const char *md5;
     const char *filename;
     const char *content_type;
     const bson_t *aliases;
     const bson_t *metadata;
     uint32_t chunk_size;
  } mongoc_gridfs_file_opt_t;

Description
-----------

This structure contains options that can be set on a :symbol:`mongoc_gridfs_file_t`. It can be used by various functions when creating a new gridfs file.

.. only:: html

  Functions
  ---------

  .. toctree::
    :titlesonly:
    :maxdepth: 1

