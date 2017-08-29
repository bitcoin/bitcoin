:man_page: mongoc_uri_get_compressors

mongoc_uri_get_compressors()
============================

Synopsis
--------

.. code-block:: c

  const bson_t *
  mongoc_uri_get_compressors (const mongoc_uri_t *uri);

Parameters
----------

* ``uri``: A :symbol:`mongoc_uri_t`.

Description
-----------

Returns a bson document with the enabled compressors as the keys.

Example
-------

.. code-block:: c

    mongoc_client_t *client;
    mongoc_uri_t *uri;

    uri = mongoc_uri_new ("mongodb://localhost/?compressors=zlib,snappy");

    if (bson_has_field (mongoc_uri_get_compressors (uri), "snappy")) {
       /* snappy enabled */
    }

Returns
-------

A bson_t * which should not be modified or freed.


