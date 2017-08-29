:man_page: mongoc_write_concern_destroy

mongoc_write_concern_destroy()
==============================

Synopsis
--------

.. code-block:: c

  void
  mongoc_write_concern_destroy (mongoc_write_concern_t *write_concern);

Parameters
----------

* ``write_concern``: A :symbol:`mongoc_write_concern_t`.

Description
-----------

Frees all resources associated with the write concern structure.

