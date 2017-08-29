:man_page: mongoc_uri_get_read_concern

mongoc_uri_get_read_concern()
=============================

Synopsis
--------

.. code-block:: c

  const mongoc_read_concern_t *
  mongoc_uri_get_read_concern (const mongoc_uri_t *uri);

Parameters
----------

* ``uri``: A :symbol:`mongoc_uri_t`.

Description
-----------

Fetches a read concern that is owned by the URI instance. This read concern is configured based on URI parameters.

Returns
-------

Returns a :symbol:`mongoc_read_concern_t` that should not be modified or freed.

