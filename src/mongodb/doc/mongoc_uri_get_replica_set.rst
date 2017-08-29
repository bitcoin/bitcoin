:man_page: mongoc_uri_get_replica_set

mongoc_uri_get_replica_set()
============================

Synopsis
--------

.. code-block:: c

  const char *
  mongoc_uri_get_replica_set (const mongoc_uri_t *uri);

Parameters
----------

* ``uri``: A :symbol:`mongoc_uri_t`.

Description
-----------

Fetches the ``replicaSet`` parameter of an URI.

Returns
-------

Returns a string which should not be modified or freed.

