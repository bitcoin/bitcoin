:man_page: mongoc_read_prefs_destroy

mongoc_read_prefs_destroy()
===========================

Synopsis
--------

.. code-block:: c

  void
  mongoc_read_prefs_destroy (mongoc_read_prefs_t *read_prefs);

Parameters
----------

* ``read_prefs``: A :symbol:`mongoc_read_prefs_t`.

Description
-----------

Frees a read preference and all associated resources.

