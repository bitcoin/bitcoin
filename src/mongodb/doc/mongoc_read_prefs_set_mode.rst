:man_page: mongoc_read_prefs_set_mode

mongoc_read_prefs_set_mode()
============================

Synopsis
--------

.. code-block:: c

  void
  mongoc_read_prefs_set_mode (mongoc_read_prefs_t *read_prefs,
                              mongoc_read_mode_t mode);

Parameters
----------

* ``read_prefs``: A :symbol:`mongoc_read_prefs_t`.
* ``mode``: A :symbol:`mongoc_read_mode_t`.

Description
-----------

Sets the read preference mode. See the MongoDB website for more information on `Read Preferences <http://docs.mongodb.org/manual/core/read-preference/>`_.

