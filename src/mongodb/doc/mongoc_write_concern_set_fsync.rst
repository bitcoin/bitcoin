:man_page: mongoc_write_concern_set_fsync

mongoc_write_concern_set_fsync()
================================

Synopsis
--------

.. code-block:: c

  void
  mongoc_write_concern_set_fsync (mongoc_write_concern_t *write_concern,
                                  bool fsync_);

Parameters
----------

* ``write_concern``: A :symbol:`mongoc_write_concern_t`.
* ``fsync_``: A boolean.

Description
-----------

Sets if a fsync must be performed before indicating write success.

Deprecated
----------

.. warning::

  The ``fsync`` write concern is deprecated.

Please use :symbol:`mongoc_write_concern_set_journal()` instead.

