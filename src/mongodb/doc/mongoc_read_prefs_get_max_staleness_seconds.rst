:man_page: mongoc_read_prefs_get_max_staleness_seconds

mongoc_read_prefs_get_max_staleness_seconds()
=============================================

Synopsis
--------

.. code-block:: c

  int64_t
  mongoc_read_prefs_get_max_staleness_seconds (
     const mongoc_read_prefs_t *read_prefs);

Parameters
----------

* ``read_prefs``: A :symbol:`mongoc_read_prefs_t`.

Description
-----------

Clients estimate the staleness of each secondary, and select for reads only those secondaries whose estimated staleness is less than or equal to maxStalenessSeconds. The default is ``MONGOC_NO_MAX_STALENESS``.

