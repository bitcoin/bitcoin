:man_page: mongoc_read_concern_copy

mongoc_read_concern_copy()
==========================

Synopsis
--------

.. code-block:: c

  mongoc_read_concern_t *
  mongoc_read_concern_copy (const mongoc_read_concern_t *read_concern);

Parameters
----------

* ``read_concern``: A :symbol:`mongoc_read_concern_t`.

Description
-----------

Performs a deep copy of ``read_concern``.

Returns
-------

Returns a newly allocated copy of ``read_concern`` that should be freed with :symbol:`mongoc_read_concern_destroy()` when no longer in use.

