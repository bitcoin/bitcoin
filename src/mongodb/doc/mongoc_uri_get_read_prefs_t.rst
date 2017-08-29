:man_page: mongoc_uri_get_read_prefs_t

mongoc_uri_get_read_prefs_t()
=============================

Synopsis
--------

.. code-block:: c

  const mongoc_read_prefs_t *
  mongoc_uri_get_read_prefs_t (const mongoc_uri_t *uri);

Parameters
----------

* ``uri``: A :symbol:`mongoc_uri_t`.

Description
-----------

Fetches a read preference that is owned by the URI instance. This read preference concern is configured based on URI parameters.

Returns
-------

Returns a :symbol:`mongoc_read_prefs_t` that should not be modified or freed.

