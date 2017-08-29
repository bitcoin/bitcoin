:man_page: mongoc_cursor_set_limit

mongoc_cursor_set_limit()
=========================

Synopsis
--------

.. code-block:: c

  bool
  mongoc_cursor_set_limit (mongoc_cursor_t *cursor, int64_t limit);

Parameters
----------

* ``cursor``: A :symbol:`mongoc_cursor_t`.
* ``limit``: The maximum number of documents to retrieve for this query.

Description
-----------

Limits the number of documents in the result set.

This function is useful for setting the limit on a cursor after the cursor is created, but before any calls to :symbol:`mongoc_cursor_next`. It can also be used to pass a negative limit: The ``limit`` parameter to ``mongoc_cursor_set_limit`` is signed, although for backward-compatibility reasons the ``limit`` parameter to :symbol:`mongoc_collection_find` is not.

Calling this function after :symbol:`mongoc_cursor_next` has no effect.

Returns
-------

Returns true on success. If any arguments are invalid, returns false and leaves the limit unchanged.
