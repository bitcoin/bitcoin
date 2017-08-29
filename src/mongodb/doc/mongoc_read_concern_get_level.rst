:man_page: mongoc_read_concern_get_level

mongoc_read_concern_get_level()
===============================

Synopsis
--------

.. code-block:: c

  const char *
  mongoc_read_concern_get_level (const mongoc_read_concern_t *read_concern);

Parameters
----------

* ``read_concern``: A :symbol:`mongoc_read_concern_t`.

Description
-----------

Returns the currently set read concern.

Returns
-------

Returns the current readConcern. If none is set, returns NULL.
