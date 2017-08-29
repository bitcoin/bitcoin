:man_page: mongoc_matcher
:orphan:

Client Side Document Matching
=============================

Basic Document Matching (Deprecated)
------------------------------------

.. warning::

  This feature will be removed in version 2.0.

The MongoDB C driver supports matching a subset of the MongoDB query specification on the client.

Currently, basic numeric, string, subdocument, and array equality, ``$gt``, ``$gte``, ``$lt``, ``$lte``, ``$in``, ``$nin``, ``$ne``, ``$exists``, ``$type``, ``$and``, and ``$or`` are supported. As this is not the same implementation as the MongoDB server, some inconsistencies may occur. Please file a bug if you find such a case.

The following example performs a basic query against a BSON document.

.. literalinclude:: ../examples/example-matcher.c
   :language: c
   :caption: example-matcher.c

The following example shows how to process a BSON stream from ``stdin`` and match it against a query. This can be useful if you need to perform simple matching against ``mongodump`` backups.

.. literalinclude:: ../examples/filter-bsondump.c
   :language: c
   :caption: filter-bsondump.c

To test this, perform a ``mongodump`` of a single collection and pipe it to the program.

.. code-block:: none

  $ echo "db.test.insert({hello:'world'})" | mongoMongoDB shell version: 2.6.1
  connecting to: test
  WriteResult({ "nInserted" : 1 })
  bye$ mongodump -d test -c test -o - | filter-bsondump{ "_id" : { "$oid" : "537afac9a70e5b4d556153bc" }, "hello" : "world" }

