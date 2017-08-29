:man_page: mongoc_cursor_get_batch_size

mongoc_cursor_get_batch_size()
==============================

Synopsis
--------

.. code-block:: c

  uint32_t
  mongoc_cursor_get_batch_size (const mongoc_cursor_t *cursor);

Parameters
----------

* ``cursor``: A :symbol:`mongoc_cursor_t`.

Description
-----------

Retrieve the cursor's batch size, the maximum number of documents returned per round trip to the server. A batch size of zero means the cursor accepts the server's maximum batch size.

See `Cursor Batches <https://docs.mongodb.org/manual/core/cursors/#cursor-batches>`_ in the MongoDB Manual.

