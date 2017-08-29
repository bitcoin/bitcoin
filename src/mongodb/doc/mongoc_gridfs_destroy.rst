:man_page: mongoc_gridfs_destroy

mongoc_gridfs_destroy()
=======================

Synopsis
--------

.. code-block:: c

  void
  mongoc_gridfs_destroy (mongoc_gridfs_t *gridfs);

Parameters
----------

* ``gridfs``: A :symbol:`mongoc_gridfs_t`.

Description
-----------

This function shall destroy the gridfs structure referenced by ``gridfs`` and any resources associated with the gridfs.

