:man_page: mongoc_write_concern_is_acknowledged

mongoc_write_concern_is_acknowledged()
======================================

Synopsis
--------

.. code-block:: c

  bool
  mongoc_write_concern_is_acknowledged (
     const mongoc_write_concern_t *write_concern);

Parameters
----------

* ``write_concern``: A :symbol:`mongoc_write_concern_t`.

Description
-----------

Test if this is an acknowledged or unacknowledged write concern.

If ``write_concern`` is NULL, returns true. (In other words, writes are acknowledged by default.)

