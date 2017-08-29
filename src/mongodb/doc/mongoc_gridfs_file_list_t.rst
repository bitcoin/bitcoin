:man_page: mongoc_gridfs_file_list_t

mongoc_gridfs_file_list_t
=========================

Synopsis
--------

.. code-block:: c

  #include <mongoc.h>

  typedef struct _mongoc_gridfs_file_list_t mongoc_gridfs_file_list_t;

Description
-----------

``mongoc_gridfs_file_list_t`` provides a gridfs file list abstraction.  It provides iteration and basic marshalling on top of a regular :symbol:`mongoc_collection_find_with_opts()` style query. In interface, it's styled after :symbol:`mongoc_cursor_t`.

Example
-------

.. code-block:: c

  mongoc_gridfs_file_list_t *list;
  mongoc_gridfs_file_t *file;

  list = mongoc_gridfs_find (gridfs, query);

  while ((file = mongoc_gridfs_file_list_next (list))) {
     do_something (file);

     mongoc_gridfs_file_destroy (file);
  }

  mongoc_gridfs_file_list_destroy (list);

.. only:: html

  Functions
  ---------

  .. toctree::
    :titlesonly:
    :maxdepth: 1

    mongoc_gridfs_file_list_destroy
    mongoc_gridfs_file_list_error
    mongoc_gridfs_file_list_next

