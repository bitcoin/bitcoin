:man_page: mongoc_client_set_read_prefs

mongoc_client_set_read_prefs()
==============================

Synopsis
--------

.. code-block:: c

  void
  mongoc_client_set_read_prefs (mongoc_client_t *client,
                                const mongoc_read_prefs_t *read_prefs);

Sets the default read preferences to use with future operations upon ``client``.

The global default is to read from the replica set primary.

It is a programming error to call this function on a client from a :symbol:`mongoc_client_pool_t`. For pooled clients, set the read preferences with the :ref:`MongoDB URI <mongoc_uri_t_read_prefs_options>` instead.

Please see the MongoDB website for a description of `Read Preferences <http://docs.mongodb.org/manual/core/read-preference/>`_.

Parameters
----------

* ``client``: A :symbol:`mongoc_client_t`.
* ``read_prefs``: A :symbol:`mongoc_read_prefs_t`.

