:man_page: mongoc_read_concern_destroy

mongoc_read_concern_destroy()
=============================

Synopsis
--------

.. code-block:: c

  void
  mongoc_read_concern_destroy (mongoc_read_concern_t *read_concern);

Parameters
----------

* ``read_concern``: A :symbol:`mongoc_read_concern_t`.

Description
-----------

Frees all resources associated with the read concern structure.

