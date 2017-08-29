:man_page: mongoc_cursor_get_max_await_time_ms

mongoc_cursor_get_max_await_time_ms()
=====================================

Synopsis
--------

.. code-block:: c

  uint32_t
  mongoc_cursor_get_max_await_time_ms (mongoc_cursor_t *cursor);

Parameters
----------

* ``cursor``: A :symbol:`mongoc_cursor_t`.

Description
-----------

Retrieve the value set with :symbol:`mongoc_cursor_set_max_await_time_ms`.

