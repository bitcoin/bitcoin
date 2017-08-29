:man_page: mongoc_uri_get_password

mongoc_uri_get_password()
=========================

Synopsis
--------

.. code-block:: c

  const char *
  mongoc_uri_get_password (const mongoc_uri_t *uri);

Parameters
----------

* ``uri``: A :symbol:`mongoc_uri_t`.

Description
-----------

Fetches the password portion of an URI.

Returns
-------

A string which should not be modified or freed.

