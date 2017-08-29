:man_page: mongoc_basic_troubleshooting

Basic Troubleshooting
=====================

Troubleshooting Checklist
-------------------------

The following is a short list of things to check when you have a problem.

* Did you call ``mongoc_init()`` in ``main()``? If not, you will likely see a segfault.
* Have you leaked any clients or cursors as can be found with ``mongoc-stat <PID>``?
* Have packets been delivered to the server? See egress bytes from ``mongoc-stat <PID>``.
* Does ``valgrind`` show any leaks? Ensure you call ``mongoc_cleanup()`` at the end of your process to cleanup lingering allocations from the MongoDB C driver.
* If compiling your own copy of MongoDB C driver, consider configuring with ``--enable-tracing`` to enable function tracing and hex dumps of network packets to ``STDERR`` and ``STDOUT``.

Performance Counters
--------------------

The MongoDB C driver comes with a unique feature to help developers and sysadmins troubleshoot problems in production.
Performance counters are available for each process using the driver.
The counters can be accessed outside of the application process via a shared memory segment.
This means that you can graph statistics about your application process easily from tools like Munin or Nagios.
Your author often uses ``watch --interval=0.5 -d mongoc-stat $PID`` to monitor an application.
      
Counters are currently available on UNIX-like platforms that support shared memory segments.

* Active and Disposed Cursors
* Active and Disposed Clients, Client Pools, and Socket Streams.
* Number of operations sent and received, by type.
* Bytes transferred and received.
* Authentication successes and failures.
* Number of wire protocol errors.

To access counters for a given process, simply provide the process id to the ``mongoc-stat`` program installed with the MongoDB C Driver.

.. code-block:: none

  $ mongoc-stat 22203
     Operations : Egress Total        : The number of sent operations.                    : 13247
     Operations : Ingress Total       : The number of received operations.                : 13246
     Operations : Egress Queries      : The number of sent Query operations.              : 13247
     Operations : Ingress Queries     : The number of received Query operations.          : 0
     Operations : Egress GetMore      : The number of sent GetMore operations.            : 0
     Operations : Ingress GetMore     : The number of received GetMore operations.        : 0
     Operations : Egress Insert       : The number of sent Insert operations.             : 0
     Operations : Ingress Insert      : The number of received Insert operations.         : 0
     Operations : Egress Delete       : The number of sent Delete operations.             : 0
     Operations : Ingress Delete      : The number of received Delete operations.         : 0
     Operations : Egress Update       : The number of sent Update operations.             : 0
     Operations : Ingress Update      : The number of received Update operations.         : 0
     Operations : Egress KillCursors  : The number of sent KillCursors operations.        : 0
     Operations : Ingress KillCursors : The number of received KillCursors operations.    : 0
     Operations : Egress Msg          : The number of sent Msg operations.                : 0
     Operations : Ingress Msg         : The number of received Msg operations.            : 0
     Operations : Egress Reply        : The number of sent Reply operations.              : 0
     Operations : Ingress Reply       : The number of received Reply operations.          : 13246
        Cursors : Active              : The number of active cursors.                     : 1
        Cursors : Disposed            : The number of disposed cursors.                   : 13246
        Clients : Active              : The number of active clients.                     : 1
        Clients : Disposed            : The number of disposed clients.                   : 0
        Streams : Active              : The number of active streams.                     : 1
        Streams : Disposed            : The number of disposed streams.                   : 0
        Streams : Egress Bytes        : The number of bytes sent.                         : 794931
        Streams : Ingress Bytes       : The number of bytes received.                     : 589694
        Streams : N Socket Timeouts   : The number of socket timeouts.                    : 0
   Client Pools : Active              : The number of active client pools.                : 1
   Client Pools : Disposed            : The number of disposed client pools.              : 0
       Protocol : Ingress Errors      : The number of protocol errors on ingress.         : 0
           Auth : Failures            : The number of failed authentication requests.     : 0
           Auth : Success             : The number of successful authentication requests. : 0

.. _basic-troubleshooting_file_bug:

Submitting a Bug Report
-----------------------

Think you've found a bug? Want to see a new feature in the MongoDB C driver? Please open a case in our issue management tool, JIRA:

* `Create an account and login <https://jira.mongodb.org>`_.
* Navigate to `the CDRIVER project <https://jira.mongodb.org/browse/CDRIVER>`_.
* Click *Create Issue* - Please provide as much information as possible about the issue type and how to reproduce it.

Bug reports in JIRA for all driver projects (i.e. CDRIVER, CSHARP, JAVA) and the Core Server (i.e. SERVER) project are *public*.

