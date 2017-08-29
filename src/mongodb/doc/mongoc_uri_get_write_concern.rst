:man_page: mongoc_uri_get_write_concern

mongoc_uri_get_write_concern()
==============================

Synopsis
--------

.. code-block:: c

  const mongoc_write_concern_t *
  mongoc_uri_get_write_concern (const mongoc_uri_t *uri);

Parameters
----------

* ``uri``: A :symbol:`mongoc_uri_t`.

Description
-----------

Fetches a write concern that is owned by the URI instance. This write concern is configured based on URI parameters.

Returns
-------

Returns a :symbol:`mongoc_write_concern_t` that should not be modified or freed.

