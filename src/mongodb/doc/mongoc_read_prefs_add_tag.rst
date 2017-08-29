:man_page: mongoc_read_prefs_add_tag

mongoc_read_prefs_add_tag()
===========================

Synopsis
--------

.. code-block:: c

  void
  mongoc_read_prefs_add_tag (mongoc_read_prefs_t *read_prefs, const bson_t *tag);

Parameters
----------

* ``read_prefs``: A :symbol:`mongoc_read_prefs_t`.
* ``tag``: A :symbol:`bson:bson_t`.

Description
-----------

This function shall add a tag to a read preference.

