:man_page: mongoc_server_description_host

mongoc_server_description_host()
================================

Synopsis
--------

.. code-block:: c

  mongoc_host_list_t *
  mongoc_server_description_host (const mongoc_server_description_t *description);

Parameters
----------

* ``description``: A :symbol:`mongoc_server_description_t`.

Description
-----------

Return the server's host and port. This object is owned by the server description.

Returns
-------

A reference to the server description's :symbol:`mongoc_host_list_t`, which you must not modify or free.

