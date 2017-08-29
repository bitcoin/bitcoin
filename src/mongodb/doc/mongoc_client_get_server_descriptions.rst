:man_page: mongoc_client_get_server_descriptions

mongoc_client_get_server_descriptions()
=======================================

Synopsis
--------

.. code-block:: c

  mongoc_server_description_t **
  mongoc_client_get_server_descriptions (const mongoc_client_t *client,
                                         size_t *n);

Fetches an array of :symbol:`mongoc_server_description_t` structs for all known servers in the topology. Returns no servers until the client connects. Returns a single server if the client is directly connected, or all members of a replica set if the client's MongoDB URI includes a "replicaSet" option, or all known mongos servers if the MongoDB URI includes a list of them.

.. code-block:: c

  void
  show_servers (const mongoc_client_t *client)
  {
     bson_t *b = BCON_NEW ("ping", BCON_INT32 (1));
     bson_error_t error;
     bool r;
     mongoc_server_description_t **sds;
     size_t i, n;

     /* ensure client has connected */
     r = mongoc_client_command_simple (client, "db", b, NULL, NULL, &error);
     if (!r) {
        MONGOC_ERROR ("could not connect: %s\n", error.message);
        return;
     }

     sds = mongoc_client_get_server_descriptions (client, &n);

     for (i = 0; i < n; ++i) {
        printf ("%s\n", mongoc_server_description_host (sds[i])->host_and_port);
     }

     mongoc_server_descriptions_destroy_all (sds, n);
     bson_destroy (&b);
  }

Parameters
----------

* ``client``: A :symbol:`mongoc_client_t`.
* ``n``: Receives the length of the descriptions array.

Returns
-------

A newly allocated array that must be freed with :symbol:`mongoc_server_descriptions_destroy_all`.

