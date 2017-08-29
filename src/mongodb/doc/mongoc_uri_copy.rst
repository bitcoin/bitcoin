:man_page: mongoc_uri_copy

mongoc_uri_copy()
=================

Synopsis
--------

.. code-block:: c

  mongoc_uri_t *
  mongoc_uri_copy (const mongoc_uri_t *uri);

Parameters
----------

* ``uri``: A :symbol:`mongoc_uri_t`.

Description
-----------

Copies the entire contents of a URI.

Returns
-------

A newly allocated :symbol:`mongoc_uri_t` that should be freed with :symbol:`mongoc_uri_destroy()`.

