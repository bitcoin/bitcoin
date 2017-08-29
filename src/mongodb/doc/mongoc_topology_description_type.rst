:man_page: mongoc_topology_description_type

mongoc_topology_description_type()
==================================

Synopsis
--------

.. code-block:: c

  const char *
  mongoc_topology_description_type (const mongoc_topology_description_t *td);

Parameters
----------

* ``td``: A :symbol:`mongoc_topology_description_t`.

Description
-----------

This function returns a string, one of the topology types defined in the Server Discovery And Monitoring Spec:

* Unknown
* Single
* Sharded
* ReplicaSetNoPrimary
* ReplicaSetWithPrimary

