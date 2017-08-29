:man_page: mongoc_read_prefs_new

mongoc_read_prefs_new()
=======================

Synopsis
--------

.. code-block:: c

  mongoc_read_prefs_t *
  mongoc_read_prefs_new (mongoc_read_mode_t read_mode);

Parameters
----------

* ``read_mode``: A :symbol:`mongoc_read_mode_t`.

Description
-----------

Creates a new :symbol:`mongoc_read_prefs_t` using the mode specified.

Returns
-------

Returns a newly allocated :symbol:`mongoc_read_prefs_t` that should be freed with :symbol:`mongoc_read_prefs_destroy()`.

