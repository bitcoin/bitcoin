:man_page: mongoc_write_concern_get_wtag

mongoc_write_concern_get_wtag()
===============================

Synopsis
--------

.. code-block:: c

  const char *
  mongoc_write_concern_get_wtag (const mongoc_write_concern_t *write_concern);

Parameters
----------

* ``write_concern``: A :symbol:`mongoc_write_concern_t`.

Description
-----------

A string containing the wtag setting if it has been set. Otherwise returns ``NULL``.

Returns
-------

Returns a string which should not be modified or freed, or ``NULL``.

