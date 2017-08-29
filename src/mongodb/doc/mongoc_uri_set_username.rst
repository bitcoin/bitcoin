:man_page: mongoc_uri_set_username

mongoc_uri_set_username()
=========================

Synopsis
--------

.. code-block:: c

  bool
  mongoc_uri_set_username (mongoc_uri_t *uri, const char *username);

Parameters
----------

* ``uri``: A :symbol:`mongoc_uri_t`.
* ``username``: The new username.

Description
-----------

Sets the URI's username, after the URI has been parsed from a string. The driver authenticates with this username if the password is also set.

Returns
-------

Returns false if the option cannot be set, for example if ``username`` is not valid UTF-8.

