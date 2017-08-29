:man_page: mongoc_cursor_current

mongoc_cursor_current()
=======================

Synopsis
--------

.. code-block:: c

  const bson_t *
  mongoc_cursor_current (const mongoc_cursor_t *cursor);

Parameters
----------

* ``cursor``: A :symbol:`mongoc_cursor_t`.

Description
-----------

Fetches the cursors current document or ``NULL`` if there has been an error.

Returns
-------

A :symbol:`bson:bson_t` that should not be modified or freed or ``NULL``.

