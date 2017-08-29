:man_page: mongoc_uri_new_for_host_port

mongoc_uri_new_for_host_port()
==============================

Synopsis
--------

.. code-block:: c

  mongoc_uri_t *
  mongoc_uri_new_for_host_port (const char *hostname, uint16_t port);

Parameters
----------

* ``hostname``: A string containing the hostname.
* ``port``: A uint16_t.

Description
-----------

Creates a new :symbol:`mongoc_uri_t` based on the hostname and port provided.

Returns
-------

Returns a newly allocated :symbol:`mongoc_uri_t` that should be freed with :symbol:`mongoc_uri_destroy()` when no longer in use.

