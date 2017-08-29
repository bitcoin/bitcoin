:man_page: mongoc_application_performance_monitoring

Application Performance Monitoring (APM)
========================================

The MongoDB C Driver allows you to monitor all the MongoDB operations the driver executes. This event-notification system conforms to two MongoDB driver specs:

* `Command Monitoring <https://github.com/mongodb/specifications/blob/master/source/command-monitoring/command-monitoring.rst>`_: events related to all application operations.
* `SDAM Monitoring <https://github.com/mongodb/specifications/blob/master/source/server-discovery-and-monitoring/server-discovery-and-monitoring-monitoring.rst>`_: events related to the driver's Server Discovery And Monitoring logic.

To receive notifications, create a ``mongoc_apm_callbacks_t`` with :symbol:`mongoc_apm_callbacks_new`, set callbacks on it, then pass it to :symbol:`mongoc_client_set_apm_callbacks` or :symbol:`mongoc_client_pool_set_apm_callbacks`.

Command-Monitoring Example
--------------------------

.. literalinclude:: ../examples/example-command-monitoring.c
   :language: c
   :caption: example-command-monitoring.c

This example program prints:

.. code-block:: none

  Command drop started on 127.0.0.1:
  { "drop" : "test" }

  Command drop failed:
  "ns not found"

  Command insert started on 127.0.0.1:
  { "insert" : "test", "documents" : [ { "_id" : 1 } ] }

  Command insert succeeded:
  { "ok" : 1, "n" : 1 }

  Command insert started on 127.0.0.1:
  { "insert" : "test", "documents" : [ { "_id" : 1 } ] }

  Command insert succeeded:
  { "ok" : 1,
    "n" : 0,
    "writeErrors" : [ {
       "index" : 0, "code" : 11000,
       "errmsg" : "E11000 duplicate key error"
  } ] }

  started: 3
  succeeded: 2
  failed: 1

In older versions of the MongoDB wire protocol, queries and write operations are sent to the server with special `opcodes <https://docs.mongodb.org/manual/reference/mongodb-wire-protocol/#request-opcodes>`_, not as commands. To provide consistent event notifications with any MongoDB version, these legacy opcodes are reported as if they had used modern commands.

The final "insert" command is considered successful, despite the writeError, because the server replied to the overall command with ``"ok": 1``.

SDAM Monitoring Example
-----------------------

.. literalinclude:: ../examples/example-sdam-monitoring.c
   :language: c
   :caption: example-sdam-monitoring.c

Start a 3-node replica set on localhost with set name "rs" and start the program:

.. code-block:: none

  ./example-sdam-monitoring "mongodb://localhost:27017,localhost:27018/?replicaSet=rs"

This example program prints something like:

.. code-block:: none

  topology opening
  topology changed: Unknown -> ReplicaSetNoPrimary
  server opening: localhost:27017
  server opening: localhost:27018
  localhost:27017 heartbeat started
  localhost:27018 heartbeat started
  localhost:27017 heartbeat succeeded: { ... reply ... }
  server changed: localhost:27017 Unknown -> RSPrimary
  server opening: localhost:27019
  topology changed: ReplicaSetNoPrimary -> ReplicaSetWithPrimary
    new servers:
        RSPrimary localhost:27017
  localhost:27019 heartbeat started
  localhost:27018 heartbeat succeeded: { ... reply ... }
  server changed: localhost:27018 Unknown -> RSSecondary
  topology changed: ReplicaSetWithPrimary -> ReplicaSetWithPrimary
    previous servers:
        RSPrimary localhost:27017
    new servers:
        RSPrimary localhost:27017
        RSSecondary localhost:27018
  localhost:27019 heartbeat succeeded: { ... reply ... }
  server changed: localhost:27019 Unknown -> RSSecondary
  topology changed: ReplicaSetWithPrimary -> ReplicaSetWithPrimary
    previous servers:
        RSPrimary localhost:27017
        RSSecondary localhost:27018
    new servers:
        RSPrimary localhost:27017
        RSSecondary localhost:27018
        RSSecondary localhost:27019
  topology closed

  Events:
     server changed: 3
     server opening: 3
     server closed: 0
     topology changed: 4
     topology opening: 1
     topology closed: 1
     heartbeat started: 3
     heartbeat succeeded: 3
     heartbeat failed: 0

The driver discovers the third member, "localhost:27019", and adds it to the topology.


.. only:: html

  Functions
  ---------

  .. toctree::
    :titlesonly:
    :maxdepth: 1

    mongoc_apm_callbacks_t
    mongoc_apm_command_failed_t
    mongoc_apm_command_started_t
    mongoc_apm_command_succeeded_t
    mongoc_apm_server_changed_t
    mongoc_apm_server_closed_t
    mongoc_apm_server_heartbeat_failed_t
    mongoc_apm_server_heartbeat_started_t
    mongoc_apm_server_heartbeat_succeeded_t
    mongoc_apm_server_opening_t
    mongoc_apm_topology_changed_t
    mongoc_apm_topology_closed_t
    mongoc_apm_topology_opening_t
