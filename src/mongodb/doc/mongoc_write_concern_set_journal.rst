:man_page: mongoc_write_concern_set_journal

mongoc_write_concern_set_journal()
==================================

Synopsis
--------

.. code-block:: c

  void
  mongoc_write_concern_set_journal (mongoc_write_concern_t *write_concern,
                                    bool journal);

Parameters
----------

* ``write_concern``: A :symbol:`mongoc_write_concern_t`.
* ``journal``: A boolean.

Description
-----------

Sets if the write must have been journaled before indicating success.

