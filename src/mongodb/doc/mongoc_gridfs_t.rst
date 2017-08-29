:man_page: mongoc_gridfs_t

mongoc_gridfs_t
===============

Synopsis
--------

.. code-block:: c

  #include <mongoc.h>

  typedef struct _mongoc_gridfs_t mongoc_gridfs_t;

Description
-----------

``mongoc_gridfs_t`` provides a MongoDB gridfs implementation. The system as a whole is made up of ``gridfs`` objects, which contain ``gridfs_files`` and ``gridfs_file_lists``.  Essentially, a basic file system API.

There are extensive caveats about the kind of use cases gridfs is practical for. In particular, any writing after initial file creation is likely to both break any concurrent readers and be quite expensive. That said, this implementation does allow for arbitrary writes to existing gridfs object, just use them with caution.

mongoc_gridfs also integrates tightly with the :symbol:`mongoc_stream_t` abstraction, which provides some convenient wrapping for file creation and reading/writing.  It can be used without, but its worth looking to see if your problem can fit that model.

.. warning::

  ``mongoc_gridfs_t`` does not support read preferences. In a replica set, GridFS queries are always routed to the primary.

Thread Safety
-------------

``mongoc_gridfs_t`` is NOT thread-safe and should only be used in the same thread as the owning :symbol:`mongoc_client_t`.

Lifecycle
---------

It is an error to free a ``mongoc_gridfs_t`` before freeing all related instances of :symbol:`mongoc_gridfs_file_t` and :symbol:`mongoc_gridfs_file_list_t`.

Example
-------

.. literalinclude:: ../examples/example-gridfs.c
   :language: c
   :caption: example-gridfs.c

.. only:: html

  Functions
  ---------

  .. toctree::
    :titlesonly:
    :maxdepth: 1

    mongoc_gridfs_create_file
    mongoc_gridfs_create_file_from_stream
    mongoc_gridfs_destroy
    mongoc_gridfs_drop
    mongoc_gridfs_find
    mongoc_gridfs_find_one
    mongoc_gridfs_find_one_by_filename
    mongoc_gridfs_find_one_with_opts
    mongoc_gridfs_find_with_opts
    mongoc_gridfs_get_chunks
    mongoc_gridfs_get_files
    mongoc_gridfs_remove_by_filename

