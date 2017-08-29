:man_page: mongoc_write_concern_is_valid

mongoc_write_concern_is_valid()
===============================

Synopsis
--------

.. code-block:: c

  bool
  mongoc_write_concern_is_valid (const mongoc_write_concern_t *write_concern);

Parameters
----------

* ``write_concern``: A :symbol:`mongoc_write_concern_t`.

Description
-----------

Test if this write concern uses an invalid combination of options.

