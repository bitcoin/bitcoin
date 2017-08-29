:man_page: mongoc_uri_set_password

mongoc_uri_set_password()
=========================

Synopsis
--------

.. code-block:: c

  bool
  mongoc_uri_set_password (mongoc_uri_t *uri, const char *password);

Parameters
----------

* ``uri``: A :symbol:`mongoc_uri_t`.
* ``password``: The new password.

Description
-----------

Sets the URI's password, after the URI has been parsed from a string. The driver authenticates with this password if the username is also set.

Returns
-------

Returns false if the option cannot be set, for example if ``password`` is not valid UTF-8.

