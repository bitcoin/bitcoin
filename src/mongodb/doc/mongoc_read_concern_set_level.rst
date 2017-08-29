:man_page: mongoc_read_concern_set_level

mongoc_read_concern_set_level()
===============================

Synopsis
--------

.. code-block:: c

  bool
  mongoc_read_concern_set_level (mongoc_read_concern_t *read_concern,
                                 const char *level);

Parameters
----------

* ``read_concern``: A :symbol:`mongoc_read_concern_t`.
* ``level``: The readConcern level to use.

Description
-----------

Sets the read concern level. See :symbol:`mongoc_read_concern_t` for details.

If the struct has been used in any operation it is "frozen", and calling this function will not alter the read concern level.

Returns
-------

Returns ``true`` if the read concern level was set, or ``false`` if the struct is frozen.
