:man_page: mongoc_collection_set_write_concern

mongoc_collection_set_write_concern()
=====================================

Synopsis
--------

.. code-block:: c

  void
  mongoc_collection_set_write_concern (
     mongoc_collection_t *collection,
     const mongoc_write_concern_t *write_concern);

Parameters
----------

* ``collection``: A :symbol:`mongoc_collection_t`.
* ``write_concern``: A :symbol:`mongoc_write_concern_t`.

Description
-----------

Sets the write concern to use for operations on ``collection``.

The default write concern is MONGOC_WRITE_CONCERN_W_DEFAULT: the driver blocks awaiting basic acknowledgment of write operations from MongoDB. This is the correct write concern for the great majority of applications.

