:man_page: mongoc_aggregate

Aggregation Framework Examples
==============================

This document provides a number of practical examples that display the capabilities of the aggregation framework.

The `Aggregations using the Zip Codes Data Set <https://docs.mongodb.org/manual/tutorial/aggregation-zip-code-data-set/>`_ examples uses a publicly available data set of all zipcodes and populations in the United States. These data are available at: `zips.json <http://media.mongodb.org/zips.json>`_.

Requirements
------------

`MongoDB <https://www.mongodb.org>`_, version 2.2.0 or later. `MongoDB C driver <https://github.com/mongodb/mongo-c-driver>`_, version 0.96.0 or later.

Let's check if everything is installed.

Use the following command to load zips.json data set into mongod instance:

.. code-block:: none

  $ mongoimport --drop -d test -c zipcodes zips.json

Let's use the MongoDB shell to verify that everything was imported successfully.

.. code-block:: none

  $ mongo testMongoDB shell version: 2.6.1
  connecting to: test> db.zipcodes.count()29467> db.zipcodes.findOne(){
  	"_id" : "35004",
  	"city" : "ACMAR",
  	"loc" : [
  		-86.51557,
  		33.584132
  	],
  	"pop" : 6055,
  	"state" : "AL"
  }

Aggregations using the Zip Codes Data Set
-----------------------------------------

Each document in this collection has the following form:

.. code-block:: json

  {
    "_id" : "35004",
    "city" : "Acmar",
    "state" : "AL",
    "pop" : 6055,
    "loc" : [-86.51557, 33.584132]
  }

In these documents:

* The ``_id`` field holds the zipcode as a string.
* The ``city`` field holds the city name.
* The ``state`` field holds the two letter state abbreviation.
* The ``pop`` field holds the population.
* The ``loc`` field holds the location as a ``[latitude, longitude]`` array.

States with Populations Over 10 Million
---------------------------------------

To get all states with a population greater than 10 million, use the following aggregation pipeline:

.. literalinclude:: ../examples/aggregation/aggregation1.c
   :language: c
   :caption: aggregation1.c

You should see a result like the following:

.. code-block:: json

  { "_id" : "PA", "total_pop" : 11881643 }
  { "_id" : "OH", "total_pop" : 10847115 }
  { "_id" : "NY", "total_pop" : 17990455 }
  { "_id" : "FL", "total_pop" : 12937284 }
  { "_id" : "TX", "total_pop" : 16986510 }
  { "_id" : "IL", "total_pop" : 11430472 }
  { "_id" : "CA", "total_pop" : 29760021 }

The above aggregation pipeline is build from two pipeline operators: ``$group`` and ``$match``.

The ``$group`` pipeline operator requires _id field where we specify grouping; remaining fields specify how to generate composite value and must use one of the group aggregation functions: ``$addToSet``, ``$first``, ``$last``, ``$max``, ``$min``, ``$avg``, ``$push``, ``$sum``. The ``$match`` pipeline operator syntax is the same as the read operation query syntax.

The ``$group`` process reads all documents and for each state it creates a separate document, for example:

.. code-block:: json

  { "_id" : "WA", "total_pop" : 4866692 }

The ``total_pop`` field uses the $sum aggregation function to sum the values of all pop fields in the source documents.

Documents created by ``$group`` are piped to the ``$match`` pipeline operator. It returns the documents with the value of ``total_pop`` field greater than or equal to 10 million.

Average City Population by State
--------------------------------

To get the first three states with the greatest average population per city, use the following aggregation:

.. code-block:: c

  pipeline = BCON_NEW ("pipeline", "[",
     "{", "$group", "{", "_id", "{", "state", "$state", "city", "$city", "}", "pop", "{", "$sum", "$pop", "}", "}", "}",
     "{", "$group", "{", "_id", "$_id.state", "avg_city_pop", "{", "$avg", "$pop", "}", "}", "}",
     "{", "$sort", "{", "avg_city_pop", BCON_INT32 (-1), "}", "}",
     "{", "$limit", BCON_INT32 (3) "}",
  "]");

This aggregate pipeline produces:

.. code-block:: json

  { "_id" : "DC", "avg_city_pop" : 303450.0 }
  { "_id" : "FL", "avg_city_pop" : 27942.29805615551 }
  { "_id" : "CA", "avg_city_pop" : 27735.341099720412 }

The above aggregation pipeline is build from three pipeline operators: ``$group``, ``$sort`` and ``$limit``.

The first ``$group`` operator creates the following documents:

.. code-block:: json

  { "_id" : { "state" : "WY", "city" : "Smoot" }, "pop" : 414 }

Note, that the ``$group`` operator can't use nested documents except the ``_id`` field.

The second ``$group`` uses these documents to create the following documents:

.. code-block:: json

  { "_id" : "FL", "avg_city_pop" : 27942.29805615551 }

These documents are sorted by the ``avg_city_pop`` field in descending order. Finally, the ``$limit`` pipeline operator returns the first 3 documents from the sorted set.

