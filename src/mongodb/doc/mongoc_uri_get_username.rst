:man_page: mongoc_uri_get_username

mongoc_uri_get_username()
=========================

Synopsis
--------

.. code-block:: c

  const char *
  mongoc_uri_get_username (const mongoc_uri_t *uri);

Parameters
----------

* ``uri``: A :symbol:`mongoc_uri_t`.

Description
-----------

Fetches the username portion of a URI.

Returns
-------

Returns a string which should not be modified or freed.

