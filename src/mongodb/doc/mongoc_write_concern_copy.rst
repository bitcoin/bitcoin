:man_page: mongoc_write_concern_copy

mongoc_write_concern_copy()
===========================

Synopsis
--------

.. code-block:: c

  mongoc_write_concern_t *
  mongoc_write_concern_copy (const mongoc_write_concern_t *write_concern);

Parameters
----------

* ``write_concern``: A :symbol:`mongoc_write_concern_t`.

Description
-----------

Performs a deep copy of ``write_concern``.

Returns
-------

Returns a newly allocated copy of ``write_concern`` that should be freed with :symbol:`mongoc_write_concern_destroy()` when no longer in use.

