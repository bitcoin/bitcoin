:man_page: mongoc_cursor_get_id

mongoc_cursor_get_id()
======================

Synopsis
--------

.. code-block:: c

  int64_t
  mongoc_cursor_get_id (const mongoc_cursor_t *cursor);

Parameters
----------

* ``cursor``: A :symbol:`mongoc_cursor_t`.

Description
-----------

Retrieves the cursor id used by the server to track the cursor.

This number is zero until the driver actually uses a server when executing the query, and after it has fetched all results from the server.

