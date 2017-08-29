:man_page: mongoc_write_concern_new

mongoc_write_concern_new()
==========================

Synopsis
--------

.. code-block:: c

  mongoc_write_concern_t *
  mongoc_write_concern_new (void);

Returns
-------

Creates a newly allocated write concern that can be configured based on user preference. This should be freed with :symbol:`mongoc_write_concern_destroy()` when no longer in use.

