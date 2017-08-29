:man_page: mongoc_cursor_error

mongoc_cursor_error()
=====================

Synopsis
--------

.. code-block:: c

  bool
  mongoc_cursor_error (mongoc_cursor_t *cursor, bson_error_t *error);

Parameters
----------

* ``cursor``: A :symbol:`mongoc_cursor_t`.
* ``error``: An optional location for a :symbol:`bson_error_t <errors>` or ``NULL``.

Description
-----------

This function checks to see if an error has occurred while iterating the cursor.

Errors
------

Errors are propagated via the ``error`` parameter.

Returns
-------

false if no error has occurred, otherwise true and error is set.

.. only:: html

  .. taglist:: See Also:
    :tags: cursor-error
