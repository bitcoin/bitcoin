:man_page: mongoc_common_task_examples

Common Tasks
============

Drivers for some other languages provide helper functions to perform certain common tasks. In the C Driver we must explicitly build commands to send to the server.

This snippet contains example code for the ``explain``, ``copydb`` and ``cloneCollection`` commands.

Setup
-----

First we'll write some code to insert sample data:

.. literalinclude:: ../examples/doc-common-insert.c
   :language: c
   :caption: doc-common-insert.c

"explain" Command
-----------------

This is how to use the ``explain`` command in MongoDB 3.2+:

.. literalinclude:: ../examples/common_operations/explain.c
   :language: c
   :caption: explain.c

.. _mongoc-common-task-examples_copydb:

"copydb" Command
----------------

This example requires two instances of mongo to be running.

Here's how to use the ``copydb`` command to copy a database from another instance of MongoDB:

.. literalinclude:: ../examples/common_operations/copydb.c
   :language: c
   :caption: copydb.c

.. _mongoc-common-task-examples_clone_collection:

"cloneCollection" Command
-------------------------

This example requires two instances of mongo to be running.

Here's an example of the ``cloneCollection`` command to clone a collection from another instance of MongoDB:

.. literalinclude:: ../examples/common_operations/clone-collection.c
   :language: c
   :caption: clone-collection.c

Running the Examples
--------------------

.. literalinclude:: ../examples/common_operations/common-operations.c
   :language: c
   :caption: common-operations.c

First launch two separate instances of mongod (must be done from separate shells):

.. code-block:: none

  $ mongod

.. code-block:: none

  $ mkdir /tmp/db2$ mongod --dbpath /tmp/db2 --port 27018 # second instance

Now compile and run the example program:

.. code-block:: none

  $ cd examples/common_operations/$ gcc -Wall -o example common-operations.c $(pkg-config --cflags --libs libmongoc-1.0)$ ./example localhost:27017 localhost:27018
  Inserting data
  explain
  {
     "executionStats" : {
        "allPlansExecution" : [],
        "executionStages" : {
           "advanced" : 19,
           "direction" : "forward" ,
           "docsExamined" : 76,
           "executionTimeMillisEstimate" : 0,
           "filter" : {
              "x" : {
                 "$eq" : 1
              }
           },
           "invalidates" : 0,
           "isEOF" : 1,
           "nReturned" : 19,
           "needTime" : 58,
           "needYield" : 0,
           "restoreState" : 0,
           "saveState" : 0,
           "stage" : "COLLSCAN" ,
           "works" : 78
        },
        "executionSuccess" : true,
        "executionTimeMillis" : 0,
        "nReturned" : 19,
        "totalDocsExamined" : 76,
        "totalKeysExamined" : 0
     },
     "ok" : 1,
     "queryPlanner" : {
        "indexFilterSet" : false,
        "namespace" : "test.things",
        "parsedQuery" : {
           "x" : {
              "$eq" : 1
           }
        },
        "plannerVersion" : 1,
        "rejectedPlans" : [],
        "winningPlan" : {
           "direction" : "forward" ,
           "filter" : {
              "x" : {
                 "$eq" : 1
              }
           },
           "stage" : "COLLSCAN"
        }
     },
     "serverInfo" : {
        "gitVersion" : "05552b562c7a0b3143a729aaa0838e558dc49b25" ,
        "host" : "MacBook-Pro-57.local",
        "port" : 27017,
        "version" : "3.2.6"
     }
  }
  copydb
  { "ok" : 1 }
  clone collection
  { "ok" : 1 }

