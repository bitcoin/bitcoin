:man_page: mongoc_topology_description_t

mongoc_topology_description_t
=============================

Status of MongoDB Servers

Synopsis
--------

.. code-block:: c

  typedef struct _mongoc_topology_description_t mongoc_topology_description_t;

``mongoc_topology_description_t`` is an opaque type representing the driver's knowledge of the MongoDB server or servers it is connected to.
Its API conforms to the `SDAM Monitoring Specification <https://github.com/mongodb/specifications/blob/master/source/server-discovery-and-monitoring/server-discovery-and-monitoring-monitoring.rst>`_.
    
Applications receive a temporary reference to a ``mongoc_topology_description_t`` as a parameter to an SDAM Monitoring callback. See :doc:`Introduction to Application Performance Monitoring <application-performance-monitoring>`.
    

.. only:: html

  Functions
  ---------

  .. toctree::
    :titlesonly:
    :maxdepth: 1

    mongoc_topology_description_get_servers
    mongoc_topology_description_has_readable_server
    mongoc_topology_description_has_writable_server
    mongoc_topology_description_type

