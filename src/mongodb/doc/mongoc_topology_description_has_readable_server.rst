:man_page: mongoc_topology_description_has_readable_server

mongoc_topology_description_has_readable_server()
=================================================

Synopsis
--------

.. code-block:: c

  bool
  mongoc_topology_description_has_readable_server (
     mongoc_topology_description_t *td, const mongoc_read_prefs_t *prefs);

Determines if the topology has a readable server available.
Servers are filtered by the given read preferences only if the driver is connected to a replica set, otherwise the read preferences are ignored.
This function uses the driver's current knowledge of the state of the MongoDB server or servers it is connected to; it does no I/O and it does not block.

Parameters
----------

* ``td``: A :symbol:`mongoc_topology_description_t`.
* ``read_prefs``: A :symbol:`mongoc_read_prefs_t` or ``NULL`` for default read preferences.

Returns
-------

True if there is a known server matching ``prefs``.

See Also
--------

:doc:`Introduction to Application Performance Monitoring <application-performance-monitoring>`

