:man_page: mongoc_cursor_set_max_await_time_ms

mongoc_cursor_set_max_await_time_ms()
=====================================

Synopsis
--------

.. code-block:: c

  void
  mongoc_cursor_set_max_await_time_ms (mongoc_cursor_t *cursor,
                                       uint32_t max_await_time_ms);

Parameters
----------

* ``cursor``: A :symbol:`mongoc_cursor_t`.
* ``max_await_time_ms``: A timeout in milliseconds.

Description
-----------

The maximum amount of time for the server to wait on new documents to satisfy a tailable cursor query. Only applies if the cursor is created from :symbol:`mongoc_collection_find_with_opts` with "tailable" and "awaitData" options, and the server is MongoDB 3.2 or later. See `the documentation for maxTimeMS and the "getMore" command <https://docs.mongodb.org/master/reference/command/getMore/>`_.

The ``max_await_time_ms`` cannot be changed after the first call to :symbol:`mongoc_cursor_next`.

See Also
--------

:ref:`Tailable Cursors. <cursors_tailable>`

