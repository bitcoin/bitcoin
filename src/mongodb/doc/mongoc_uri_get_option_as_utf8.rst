:man_page: mongoc_uri_get_option_as_utf8

mongoc_uri_get_option_as_utf8()
===============================

Synopsis
--------

.. code-block:: c

  const char *
  mongoc_uri_get_option_as_utf8 (const mongoc_uri_t *uri,
                                 const char *option,
                                 const char *fallback);

Parameters
----------

* ``uri``: A :symbol:`mongoc_uri_t`.
* ``option``: The name of an option, case insensitive.
* ``fallback``: A default value to return.

Description
-----------

Returns the value of the URI option if it is set and of the correct type (string). This value is a pointer into the URI's internal buffer, and is only valid until the URI is modified or freed. If the option is not set, or set to an invalid type, returns ``fallback``.

