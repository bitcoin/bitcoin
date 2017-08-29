:man_page: mongoc_uri_get_mechanism_properties

mongoc_uri_get_mechanism_properties()
=====================================

Synopsis
--------

.. code-block:: c

  bool
  mongoc_uri_get_mechanism_properties (const mongoc_uri_t *uri,
                                       bson_t *properties /* OUT */);

Parameters
----------

* ``uri``: A :symbol:`mongoc_uri_t`.
* ``properties``: An uninitialized :symbol:`bson:bson_t`.

Description
-----------

Fetches the "authMechanismProperties" options set on this :symbol:`mongoc_uri_t`. The out-parameter ``properties`` should be an uninitialized, stack-allocated :symbol:`bson:bson_t`. It is statically initialized with :symbol:`bson:bson_init_static` to point to the internal data of ``uri``, so its contents must not be modified and it becomes invalid after ``uri`` is destroyed.

Returns
-------

If no "authMechanismProperties" have been set on ``uri``, this functions returns false and ``properties`` remains uninitialized.

Example
-------

.. code-block:: c

  mongoc_uri_t *uri;
  bson_t props;

  uri = mongoc_uri_new (
     "mongodb://user%40DOMAIN.COM:password@localhost/?authMechanism=GSSAPI"
     "&authMechanismProperties=SERVICE_NAME:other,CANONICALIZE_HOST_NAME:true");

  if (mongoc_uri_get_mechanism_properties (uri, &props)) {
     char *json = bson_as_canonical_extended_json (&props, NULL);
     printf ("%s\n", json);
     bson_free (json);
  } else {
     printf ("No authMechanismProperties.\n");
  }

This code produces the output:

.. code-block:: none

  { "SERVICE_NAME" : "other", "CANONICALIZE_HOST_NAME" : "true" }

See Also
--------

.. only:: html

  .. taglist:: See Also:
    :tags: authmechanism
