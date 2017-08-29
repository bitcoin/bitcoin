:man_page: mongoc_read_prefs_is_valid

mongoc_read_prefs_is_valid()
============================

Synopsis
--------

.. code-block:: c

  bool
  mongoc_read_prefs_is_valid (const mongoc_read_prefs_t *read_prefs);

Parameters
----------

* ``read_prefs``: A :symbol:`mongoc_read_prefs_t`.

Description
-----------

Performs a consistency check of ``read_prefs`` to ensure it makes sense and can be satisfied.

This only performs local consistency checks.

Returns
-------

Returns true if the read pref is valid.

