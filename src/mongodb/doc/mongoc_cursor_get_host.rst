:man_page: mongoc_cursor_get_host

mongoc_cursor_get_host()
========================

Synopsis
--------

.. code-block:: c

  void
  mongoc_cursor_get_host (mongoc_cursor_t *cursor, mongoc_host_list_t *host);

Parameters
----------

* ``cursor``: A :symbol:`mongoc_cursor_t`.
* ``host``: A :symbol:`mongoc_host_list_t`.

Description
-----------

Fetches the MongoDB host that the cursor is communicating with in the ``host`` out parameter.

