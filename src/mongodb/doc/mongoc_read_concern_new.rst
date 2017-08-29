:man_page: mongoc_read_concern_new

mongoc_read_concern_new()
=========================

Synopsis
--------

.. code-block:: c

  mongoc_read_concern_t *
  mongoc_read_concern_new (void);

Returns
-------

Creates a newly allocated read concern that can be configured based on user preference. This should be freed with :symbol:`mongoc_read_concern_destroy()` when no longer in use.

