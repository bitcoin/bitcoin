:man_page: mongoc_client_new_from_uri

mongoc_client_new_from_uri()
============================

Synopsis
--------

.. code-block:: c

  mongoc_client_t *
  mongoc_client_new_from_uri (const mongoc_uri_t *uri);

Creates a new :symbol:`mongoc_client_t` using the :symbol:`mongoc_uri_t` provided.

Parameters
----------

* ``uri``: A :symbol:`mongoc_uri_t`.

Returns
-------

A newly allocated :symbol:`mongoc_client_t` that should be freed with :symbol:`mongoc_client_destroy()` when no longer in use.

