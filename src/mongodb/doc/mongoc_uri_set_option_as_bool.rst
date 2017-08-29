:man_page: mongoc_uri_set_option_as_bool

mongoc_uri_set_option_as_bool()
===============================

Synopsis
--------

.. code-block:: c

  bool
  mongoc_uri_set_option_as_bool (const mongoc_uri_t *uri,
                                 const char *option,
                                 bool value);

Parameters
----------

* ``uri``: A :symbol:`mongoc_uri_t`.
* ``option``: The name of an option, case insensitive.
* ``value``: The new value.

Description
-----------

Sets an individual URI option, after the URI has been parsed from a string.

Only known options of type bool can be set.

Updates the option in-place if already set, otherwise appends it to the URI's :symbol:`bson:bson_t` of options.

Returns
-------

True if successfully set (the named option is a known option of type bool).

