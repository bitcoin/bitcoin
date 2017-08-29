:man_page: mongoc_uri_get_options

mongoc_uri_get_options()
========================

Synopsis
--------

.. code-block:: c

  const bson_t *
  mongoc_uri_get_options (const mongoc_uri_t *uri);

Parameters
----------

* ``uri``: A :symbol:`mongoc_uri_t`.

Description
-----------

Fetches a bson document containing all of the options provided after the ``?`` of a URI.

Returns
-------

A :symbol:`bson:bson_t` which should not be modified or freed.

