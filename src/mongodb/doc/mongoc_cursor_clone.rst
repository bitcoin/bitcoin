:man_page: mongoc_cursor_clone

mongoc_cursor_clone()
=====================

Synopsis
--------

.. code-block:: c

  mongoc_cursor_t *
  mongoc_cursor_clone (const mongoc_cursor_t *cursor)
     BSON_GNUC_WARN_UNUSED_RESULT;

Parameters
----------

* ``cursor``: A :symbol:`mongoc_cursor_t`.

Description
-----------

This function shall create a copy of a :symbol:`mongoc_cursor_t`. The cloned cursor will be reset to the beginning of the query, and therefore the query will be re-executed on the MongoDB server when :symbol:`mongoc_cursor_next()` is called.

Returns
-------

A newly allocated :symbol:`mongoc_cursor_t` that should be freed with :symbol:`mongoc_cursor_destroy()` when no longer in use.

.. warning::

  Failure to handle the result of this function is a programming error.

