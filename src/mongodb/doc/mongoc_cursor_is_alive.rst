:man_page: mongoc_cursor_is_alive

mongoc_cursor_is_alive()
========================

Synopsis
--------

.. code-block:: c

  bool
  mongoc_cursor_is_alive (const mongoc_cursor_t *cursor);

Parameters
----------

* ``cursor``: A :symbol:`mongoc_cursor_t`.

Description
-----------

Checks to see if a cursor is in a state that allows contacting a server to check for more documents.  Note that even if false, there may be documents already retrieved that can be iterated using :symbol:`mongoc_cursor_next()`.

This is primarily useful with tailable cursors.

See also :symbol:`mongoc_cursor_more()`.

Returns
-------

true if the cursor will be able to attempt to retrieve more results from a server.

