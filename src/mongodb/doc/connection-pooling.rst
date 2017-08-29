:man_page: mongoc_connection_pooling

Connection Pooling
==================

The MongoDB C driver has two connection modes: single-threaded and pooled. Single-threaded mode is optimized for embedding the driver within languages like PHP. Multi-threaded programs should use pooled mode: this mode minimizes the total connection count, and in pooled mode a background thread monitors the MongoDB server topology, so the program need not block to scan it.

Single Mode
-----------

In single mode, your program creates a :symbol:`mongoc_client_t` directly:

.. code-block:: c

  mongoc_client_t *client = mongoc_client_new (
     "mongodb://hostA,hostB/?replicaSet=my_rs");

The client connects on demand when your program first uses it for a MongoDB operation. Using a non-blocking socket per server, it begins a check on each server concurrently, and uses the asynchronous ``poll`` or ``select`` function to receive events from the sockets, until all have responded or timed out. Put another way, in single-threaded mode the C Driver fans out to begin all checks concurrently, then fans in once all checks have completed or timed out. Once the scan completes, the client executes your program's operation and returns.

In single mode, the client re-scans the server topology roughly once per minute. If more than a minute has elapsed since the previous scan, the next operation on the client will block while the client completes its scan. This interval is configurable with ``heartbeatFrequencyMS`` in the connection string. (See :symbol:`mongoc_uri_t`.)

A single client opens one connection per server in your topology: these connections are used both for scanning the topology and performing normal operations.

Pooled Mode
-----------

To activate pooled mode, create a :symbol:`mongoc_client_pool_t`:

.. code-block:: c

  mongoc_uri_t *uri = mongoc_uri_new (
     "mongodb://hostA,hostB/?replicaSet=my_rs");

  mongoc_client_pool_t *pool = mongoc_client_pool_new (uri);

When your program first calls :symbol:`mongoc_client_pool_pop`, the pool launches a background thread for monitoring. The thread fans out and connects to all servers in the connection string, using non-blocking sockets and a simple event loop. As it receives ismaster responses from the servers, it updates its view of the server topology. Each time the thread discovers a new server it begins connecting to it, and adds the new socket to the list of non-blocking sockets in the event loop.

Each thread that executes MongoDB operations must check out a client from the pool:

.. code-block:: c

  mongoc_client_t *client = mongoc_client_pool_pop (pool);

  /* use the client for operations ... */

  mongoc_client_pool_push (pool, client);

The :symbol:`mongoc_client_t` object is not thread-safe, only the :symbol:`mongoc_client_pool_t` is.

When the driver is in pooled mode, your program's operations are unblocked as soon as monitoring discovers a usable server. For example, if a thread in your program is waiting to execute an "insert" on the primary, it is unblocked as soon as the primary is discovered, rather than waiting for all secondaries to be checked as well.

The pool opens one connection per server for monitoring, and each client opens its own connection to each server it uses for application operations. The background thread re-scans the server topology roughly every 10 seconds. This interval is configurable with ``heartbeatFrequencyMS`` in the connection string. (See :symbol:`mongoc_uri_t`.)

See :ref:`connection_pool_options` to configure pool size and behavior, and see :symbol:`mongoc_client_pool_t` for an extended example of a multi-threaded program that uses the driver in pooled mode.
