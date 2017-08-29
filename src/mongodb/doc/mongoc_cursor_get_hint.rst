:man_page: mongoc_cursor_get_hint

mongoc_cursor_get_hint()
========================

Synopsis
--------

.. code-block:: c

  uint32_t
  mongoc_cursor_get_hint (const mongoc_cursor_t *cursor);

Parameters
----------

* ``cursor``: A :symbol:`mongoc_cursor_t`.

Description
-----------

Retrieves the opaque id of the server used for the operation.

(The function name includes the old term "hint" for the sake of backward compatibility, but we now call this number a "server id".)

This number is zero until the driver actually uses a server when executing the find operation or :symbol:`mongoc_cursor_set_hint` is called.

