:man_page: mongoc_cursor_more

mongoc_cursor_more()
====================

Synopsis
--------

.. code-block:: c

  bool
  mongoc_cursor_more (mongoc_cursor_t *cursor);

Parameters
----------

* ``cursor``: A :symbol:`mongoc_cursor_t`.

Description
-----------

This function shall indicate if there is *potentially* more data to be read from the cursor. This is only useful with tailable cursors. Use :symbol:`mongoc_cursor_next` for regular cursors.

Details: ``mongoc_cursor_more`` is unreliable because it does not contact the server to see if there are actually more documents in the result set. It simply returns true if the cursor has not begun, or if it has begun and there are buffered documents in the client-side cursor, or if it has begun and the server has not yet told the cursor it is completely iterated.

This is unreliable with regular queries because it returns true for a new cursor before iteration, even if the cursor will match no documents. It is also true if the collection has been dropped on the server since the previous fetch, or if the cursor has finished its final batch and the next batch will be empty.

See also :symbol:`mongoc_cursor_is_alive()`.

Returns
-------

true if the cursor has locally-buffered documents, or if a round-trip to the server might fetch additional documents.

See Also
--------

:ref:`Tailable Cursor Example <cursors_tailable>`

