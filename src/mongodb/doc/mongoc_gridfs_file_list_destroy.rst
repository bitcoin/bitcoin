:man_page: mongoc_gridfs_file_list_destroy

mongoc_gridfs_file_list_destroy()
=================================

Synopsis
--------

.. code-block:: c

  void
  mongoc_gridfs_file_list_destroy (mongoc_gridfs_file_list_t *list);

Parameters
----------

* ``list``: A :symbol:`mongoc_gridfs_file_list_t`.

Description
-----------

Frees a ``mongoc_gridfs_file_list_t`` and releases any associated resources.

