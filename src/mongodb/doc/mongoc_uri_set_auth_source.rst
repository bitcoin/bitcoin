:man_page: mongoc_uri_set_auth_source

mongoc_uri_set_auth_source()
============================

Synopsis
--------

.. code-block:: c

  bool
  mongoc_uri_set_auth_source (mongoc_uri_t *uri, const char *value);

Parameters
----------

* ``uri``: A :symbol:`mongoc_uri_t`.
* ``value``: The new "authSource" value.

Description
-----------

Sets the "authSource" URI option, after the URI has been parsed from a string.

Updates the option in-place if already set, otherwise appends it to the URI's :symbol:`bson:bson_t` of options.

Returns
-------

Returns false if the option cannot be set, for example if ``value`` is not valid UTF-8.

.. only:: html

  .. taglist:: See Also:
    :tags: authmechanism
