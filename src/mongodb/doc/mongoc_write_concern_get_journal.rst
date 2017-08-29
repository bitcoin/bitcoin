:man_page: mongoc_write_concern_get_journal

mongoc_write_concern_get_journal()
==================================

Synopsis
--------

.. code-block:: c

  bool
  mongoc_write_concern_get_journal (const mongoc_write_concern_t *write_concern);

Parameters
----------

* ``write_concern``: A :symbol:`mongoc_write_concern_t`.

Description
-----------

Fetches if the write should be journaled before indicating success.

Returns
-------

Returns true if the write should be journaled.

