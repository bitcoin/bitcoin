:man_page: mongoc_uri_get_option_as_int32

mongoc_uri_get_option_as_int32()
================================

Synopsis
--------

.. code-block:: c

  int32
  mongoc_uri_get_option_as_int32 (const mongoc_uri_t *uri,
                                  const char *option,
                                  int32 fallback);

Parameters
----------

* ``uri``: A :symbol:`mongoc_uri_t`.
* ``option``: The name of an option, case insensitive.
* ``fallback``: A default value to return.

Description
-----------

Returns the value of the URI option if it is set and of the correct type (int32). Returns ``fallback`` if the option is not set, set to an invalid type, or zero.

Zero is considered "unset", so URIs can be constructed like so, and still accept default values:

.. code-block:: c

  bson_strdup_printf ("mongodb://localhost/?connectTimeoutMS=%d", myvalue)

If ``myvalue`` is non-zero it is the connection timeout; if it is zero the driver uses the default timeout.

