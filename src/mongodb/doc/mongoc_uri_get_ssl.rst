:man_page: mongoc_uri_get_ssl

mongoc_uri_get_ssl()
====================

Synopsis
--------

.. code-block:: c

  bool
  mongoc_uri_get_ssl (const mongoc_uri_t *uri);

Parameters
----------

* ``uri``: A :symbol:`mongoc_uri_t`.

Description
-----------

Fetches a boolean indicating if SSL was specified for use in the URI.

Returns
-------

Returns a boolean, true indicating that SSL should be used.

