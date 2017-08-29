:man_page: mongoc_uri_set_mechanism_properties

mongoc_uri_set_mechanism_properties()
=====================================

Synopsis
--------

.. code-block:: c

  bool
  mongoc_uri_set_mechanism_properties (mongoc_uri_t *uri,
                                       const bson_t *properties);

Parameters
----------

* ``uri``: A :symbol:`mongoc_uri_t`.
* ``properties``: A :symbol:`bson:bson_t` .

Description
-----------

Replaces all the options in URI's "authMechanismProperties" after the URI has been parsed from a string.

Returns
-------

Returns false if the option cannot be set, for example if ``properties`` is not valid BSON data.

Example
-------

.. code-block:: c

  mongoc_uri_t *uri;
  bson_t props = BSON_INITIALIZER;

  uri = mongoc_uri_new (
     "mongodb://user%40DOMAIN.COM:password@localhost/?authMechanism=GSSAPI"
     "&authMechanismProperties=SERVICE_NAME:other,CANONICALIZE_HOST_NAME:true");

  /* replace all options: replace service name "other" with "my_service", unset
   * "CANONICALIZE_HOST_NAME" and accept its default.
   */
  BSON_APPEND_UTF8 (&props, "SERVICE_NAME", "my_service");
  mongoc_uri_set_mechanism_properties (uri, &props);

  bson_destroy (&props);

See Also
--------

:ref:`GSSAPI (Kerberos) Authentication <authentication_kerberos>` and :symbol:`mongoc_uri_get_mechanism_properties`

.. only:: html

  .. taglist:: See Also:
    :tags: authmechanism
