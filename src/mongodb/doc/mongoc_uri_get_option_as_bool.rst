:man_page: mongoc_uri_get_option_as_bool

mongoc_uri_get_option_as_bool()
===============================

Synopsis
--------

.. code-block:: c

  bool
  mongoc_uri_get_option_as_bool (const mongoc_uri_t *uri,
                                 const char *option,
                                 bool fallback);

Parameters
----------

* ``uri``: A :symbol:`mongoc_uri_t`.
* ``option``: The name of an option, case insensitive.
* ``fallback``: A default value to return.

Description
-----------

Returns the value of the URI option if it is set and of the correct type (bool). If not set, or set to an invalid type, returns ``fallback``.

