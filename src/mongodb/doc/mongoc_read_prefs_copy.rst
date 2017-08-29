:man_page: mongoc_read_prefs_copy

mongoc_read_prefs_copy()
========================

Synopsis
--------

.. code-block:: c

  mongoc_read_prefs_t *
  mongoc_read_prefs_copy (const mongoc_read_prefs_t *read_prefs);

Parameters
----------

* ``read_prefs``: A :symbol:`mongoc_read_prefs_t`.

Description
-----------

This function shall deep copy a read preference.

Returns
-------

Returns a newly allocated read preference that should be freed with :symbol:`mongoc_read_prefs_destroy()`.

