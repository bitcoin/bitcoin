:man_page: mongoc_cursor_destroy

mongoc_cursor_destroy()
=======================

Synopsis
--------

.. code-block:: c

  void
  mongoc_cursor_destroy (mongoc_cursor_t *cursor);

Parameters
----------

* ``cursor``: A :symbol:`mongoc_cursor_t`.

Description
-----------

Frees a :symbol:`mongoc_cursor_t` and releases all associated resources. If a server-side cursor has been allocated, it will be released as well.

