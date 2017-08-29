:man_page: mongoc_read_prefs_set_max_staleness_seconds

mongoc_read_prefs_set_max_staleness_seconds()
=============================================

Synopsis
--------

.. code-block:: c

  void
  mongoc_read_prefs_set_max_staleness_seconds (mongoc_read_prefs_t *read_prefs,
                                               int64_t max_staleness_seconds);

Parameters
----------

* ``read_prefs``: A :symbol:`mongoc_read_prefs_t`.
* ``max_staleness_seconds``: A positive integer or ``MONGOC_NO_MAX_STALENESS``.

Description
-----------

Sets the maxStalenessSeconds to be used for the read preference. Clients estimate the staleness of each secondary, and select for reads only those secondaries whose estimated staleness is less than or equal to maxStalenessSeconds.

