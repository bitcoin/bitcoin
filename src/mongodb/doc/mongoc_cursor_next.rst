:man_page: mongoc_cursor_next

mongoc_cursor_next()
====================

Synopsis
--------

.. code-block:: c

  bool
  mongoc_cursor_next (mongoc_cursor_t *cursor, const bson_t **bson);

Parameters
----------

* ``cursor``: A :symbol:`mongoc_cursor_t`.
* ``bson``: A location for a :symbol:`const bson_t * <bson:bson_t>`.

Description
-----------

This function shall iterate the underlying cursor, setting ``bson`` to the next document.

This function is a blocking function.

Returns
-------

This function returns true if a valid bson document was read from the cursor. Otherwise, false if there was an error or the cursor was exhausted.

Errors can be determined with the :symbol:`mongoc_cursor_error()` function.

Lifecycle
---------

The bson objects set in this function are ephemeral and good until the next call. This means that you must copy the returned bson if you wish to retain it beyond the lifetime of a single call to :symbol:`mongoc_cursor_next()`.

