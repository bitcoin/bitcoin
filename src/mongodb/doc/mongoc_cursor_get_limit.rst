:man_page: mongoc_cursor_get_limit

mongoc_cursor_get_limit()
=========================

Synopsis
--------

.. code-block:: c

  int64_t
  mongoc_cursor_get_limit (mongoc_cursor_t *cursor);

Parameters
----------

* ``cursor``: A :symbol:`mongoc_cursor_t`.

Description
-----------

Return the value set with :symbol:`mongoc_cursor_set_limit` or :symbol:`mongoc_collection_find`.

