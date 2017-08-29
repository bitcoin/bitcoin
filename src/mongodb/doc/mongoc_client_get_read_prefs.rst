:man_page: mongoc_client_get_read_prefs

mongoc_client_get_read_prefs()
==============================

Synopsis
--------

.. code-block:: c

  const mongoc_read_prefs_t *
  mongoc_client_get_read_prefs (const mongoc_client_t *client);

Retrieves the default read preferences configured for the client instance. The result should not be modified or freed.

Parameters
----------

* ``client``: A :symbol:`mongoc_client_t`.

Returns
-------

A :symbol:`mongoc_read_prefs_t`.

