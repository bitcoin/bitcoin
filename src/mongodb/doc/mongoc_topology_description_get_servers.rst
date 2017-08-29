:man_page: mongoc_topology_description_get_servers

mongoc_topology_description_get_servers()
=========================================

Synopsis
--------

.. code-block:: c

  mongoc_server_description_t **
  mongoc_topology_description_get_servers (
     const mongoc_topology_description_t *td, size_t *n);

Fetches an array of :symbol:`mongoc_server_description_t` structs for all known servers in the topology.

Parameters
----------

* ``td``: A :symbol:`mongoc_topology_description_t`.
* ``n``: Receives the length of the descriptions array.

Returns
-------

A newly allocated array that must be freed with :symbol:`mongoc_server_descriptions_destroy_all`.

