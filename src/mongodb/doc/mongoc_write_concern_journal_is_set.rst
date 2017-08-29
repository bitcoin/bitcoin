:man_page: mongoc_write_concern_journal_is_set

mongoc_write_concern_journal_is_set()
=====================================

Synopsis
--------

.. code-block:: c

  bool
  mongoc_write_concern_journal_is_set (
     const mongoc_write_concern_t *write_concern);

Parameters
----------

* ``write_concern``: A :symbol:`mongoc_write_concern_t`.

Description
-----------

Test whether this write concern's "journal" option was explicitly set or uses the default setting.

