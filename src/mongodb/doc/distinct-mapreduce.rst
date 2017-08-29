:man_page: mongoc_distinct_mapreduce

"distinct" and "mapReduce"
==========================

This document provides some practical, simple, examples to demonstrate the ``distinct`` and ``mapReduce`` commands.

Setup
-----

First we'll write some code to insert sample data:

.. literalinclude:: ../examples/doc-common-insert.c
   :language: c
   :caption: doc-common-insert.c

"distinct" command
------------------

This is how to use the ``distinct`` command to get the distinct values of ``x`` which are greater than ``1``:

.. literalinclude:: ../examples/basic_aggregation/distinct.c
   :language: c
   :caption: distinct.c

"mapReduce" - basic example
---------------------------

A simple example using the map reduce framework. It simply adds up the number of occurrences of each "tag".

First define the ``map`` and ``reduce`` functions:

.. literalinclude:: ../examples/basic_aggregation/constants.c
   :language: c
   :caption: constants.c

Run the ``mapReduce`` command:

.. literalinclude:: ../examples/basic_aggregation/map-reduce-basic.c
   :language: c
   :caption: map-reduce-basic.c

"mapReduce" - more complicated example
--------------------------------------

You must have replica set running for this.

In this example we contact a secondary in the replica set and do an "inline" map reduce, so the results are returned immediately:

.. literalinclude:: ../examples/basic_aggregation/map-reduce-advanced.c
   :language: c
   :caption: map-reduce-advanced.c

Running the Examples
--------------------

Here's how to run the example code

.. literalinclude:: ../examples/basic_aggregation/basic-aggregation.c
   :language: c
   :caption: basic-aggregation.c

If you want to try the advanced map reduce example with a secondary, start a replica set (instructions for how to do this can be found `here <https://docs.mongodb.com/manual/tutorial/deploy-replica-set-for-testing/>`_).

Otherwise, just start an instance of MongoDB:

.. code-block:: none

  $ mongod

Now compile and run the example program:

.. code-block:: none

  $ cd examples/basic_aggregation/
  $ gcc -Wall -o agg-example basic-aggregation.c $(pkg-config --cflags --libs libmongoc-1.0)
  $ ./agg-example localhost

  Inserting data
  distinct
  Next double: 2.000000
  Next double: 3.000000
  map reduce
  { "result" : "outCollection", "timeMillis" : 155, "counts" : { "input" : 84, "emit" : 126, "reduce" : 3, "output" : 3 }, "ok" : 1 }
  { "_id" : "cat", "value" : 63 }
  { "_id" : "dog", "value" : 42 }
  { "_id" : "mouse", "value" : 21 }
  more complicated map reduce
  { "results" : [ { "_id" : "cat", "value" : 63 }, { "_id" : "dog", "value" : 42 }, { "_id" : "mouse", "value" : 21 } ], "timeMillis" : 14, "counts" : { "input" : 84, "emit" : 126, "reduce" : 3, "output" : 3 }, "ok" : 1 }

