:man_page: mongoc_bulk

Bulk Write Operations
=====================

This tutorial explains how to take advantage of MongoDB C driver bulk write operation features. Executing write operations in batches reduces the number of network round trips, increasing write throughput.

Bulk Insert
-----------

First we need to fetch a bulk operation handle from the :symbol:`mongoc_collection_t`. This can be performed in either ordered or unordered mode. Unordered mode allows for greater parallelization when working with sharded clusters.

.. code-block:: c

  mongoc_bulk_operation_t *bulk =
     mongoc_collection_create_bulk_operation (collection, true, write_concern);

We can now start inserting documents to the bulk operation. These will be buffered until we execute the operation.

The bulk operation will coalesce insertions as a single batch for each consecutive call to :symbol:`mongoc_bulk_operation_insert()`. This creates a pipelined effect when possible.

.. tip::

  The bulk operation API will automatically handle MongoDB servers < 2.6 by speaking the old wire protocol. However, some performance degradation may occur.

To execute the bulk operation and receive the result we call :symbol:`mongoc_bulk_operation_execute()`.

.. literalinclude:: ../examples/bulk/bulk1.c
   :language: c
   :caption: bulk1.c

Example ``reply`` document:

.. code-block:: none

  {"nInserted"   : 10000,
   "nMatched"    : 0,
   "nModified"   : 0,
   "nRemoved"    : 0,
   "nUpserted"   : 0,
   "writeErrors" : []
   "writeConcernErrors" : [] }

Mixed Bulk Write Operations
---------------------------

MongoDB C driver also supports executing mixed bulk write operations. A batch of insert, update, and remove operations can be executed together using the bulk write operations API.

.. tip::

  Though the following API will work with all versions of MongoDB, it is designed to be used with MongoDB versions >= 2.6. Much better bulk insert performance can be achieved with older versions of MongoDB through the deprecated :symbol:`mongoc_collection_insert_bulk()` method.

Ordered Bulk Write Operations
-----------------------------

Ordered bulk write operations are batched and sent to the server in the order provided for serial execution. The ``reply`` document describes the type and count of operations performed.

.. literalinclude:: ../examples/bulk/bulk2.c
   :language: c
   :caption: bulk2.c

Example ``reply`` document:

.. code-block:: none

  { "nInserted"   : 3,
    "nMatched"    : 2,
    "nModified"   : 2,
    "nRemoved"    : 10000,
    "nUpserted"   : 1,
    "upserted"    : [{"index" : 5, "_id" : 4}],
    "writeErrors" : []
    "writeConcernErrors" : [] }

The ``index`` field in the ``upserted`` array is the 0-based index of the upsert operation; in this example, the sixth operation of the overall bulk operation was an upsert, so its index is 5.

``nModified`` is only reported when using MongoDB 2.6 and later, otherwise the field is omitted.

Unordered Bulk Write Operations
-------------------------------

Unordered bulk write operations are batched and sent to the server in *arbitrary order* where they may be executed in parallel. Any errors that occur are reported after all operations are attempted.

In the next example the first and third operations fail due to the unique constraint on ``_id``. Since we are doing unordered execution the second and fourth operations succeed.

.. literalinclude:: ../examples/bulk/bulk3.c
   :language: c
   :caption: bulk3.c

Example ``reply`` document:

.. code-block:: none

  { "nInserted"    : 0,
    "nMatched"     : 1,
    "nModified"    : 1,
    "nRemoved"     : 1,
    "nUpserted"    : 0,
    "writeErrors"  : [
      { "index"  : 0,
        "code"   : 11000,
        "errmsg" : "E11000 duplicate key error index: test.test.$_id_ dup key: { : 1 }" },
      { "index"  : 2,
        "code"   : 11000,
        "errmsg" : "E11000 duplicate key error index: test.test.$_id_ dup key: { : 3 }" } ],
    "writeConcernErrors" : [] }

  Error: E11000 duplicate key error index: test.test.$_id_ dup key: { : 1 }

The :symbol:`bson_error_t <errors>` domain is ``MONGOC_ERROR_COMMAND`` and its code is 11000. 

.. _bulk_operation_bypassing_document_validation:

Bulk Operation Bypassing Document Validation
--------------------------------------------

This feature is only available when using MongoDB 3.2 and later.

By default bulk operations are validated against the schema, if any is defined. In certain cases however it may be necessary to bypass the document validation.

.. literalinclude:: ../examples/bulk/bulk5.c
   :language: c
   :caption: bulk5.c

Running the above example will result in:

.. code-block:: none

  { "nInserted" : 0,
    "nMatched" : 0,
    "nModified" : 0,
    "nRemoved" : 0,
    "nUpserted" : 0,
    "writeErrors" : [
      { "index" : 0,
        "code" : 121,
        "errmsg" : "Document failed validation" } ] }

  Error: Document failed validation

  { "nInserted" : 2,
    "nMatched" : 0,
    "nModified" : 0,
    "nRemoved" : 0,
    "nUpserted" : 0,
    "writeErrors" : [] }

The :symbol:`bson_error_t <errors>` domain is ``MONGOC_ERROR_COMMAND``.

Bulk Operation Write Concerns
-----------------------------

By default bulk operations are executed with the :symbol:`write_concern <mongoc_write_concern_t>` of the collection they are executed against. A custom write concern can be passed to the :symbol:`mongoc_collection_create_bulk_operation()` method. Write concern errors (e.g. wtimeout) will be reported after all operations are attempted, regardless of execution order.

.. literalinclude:: ../examples/bulk/bulk4.c
   :language: c
   :caption: bulk4.c

Example ``reply`` document and error message:

.. code-block:: none

  { "nInserted"    : 2,
    "nMatched"     : 0,
    "nModified"    : 0,
    "nRemoved"     : 0,
    "nUpserted"    : 0,
    "writeErrors"  : [],
    "writeConcernErrors" : [
      { "code"   : 64,
        "errmsg" : "waiting for replication timed out" }
  ] }

  Error: waiting for replication timed out

The :symbol:`bson_error_t <errors>` domain is ``MONGOC_ERROR_WRITE_CONCERN`` if there are write concern errors and no write errors. Write errors indicate failed operations, so they take precedence over write concern errors, which mean merely that the write concern is not satisfied *yet*.

Setting Collation Order
-----------------------

This feature is only available when using MongoDB 3.4 and later.

.. literalinclude:: ../examples/bulk/bulk-collation.c
   :language: c
   :caption: bulk-collation.c

Running the above example will result in:

.. code-block:: none

  { "nInserted" : 2,
     "nMatched" : 1,
     "nModified" : 1,
     "nRemoved" : 0,
     "nUpserted" : 0,
     "writeErrors" : [  ]
  }

Unacknowledged Bulk Writes
--------------------------

Set "w" to zero for an unacknowledged write. The driver sends unacknowledged writes using the legacy opcodes ``OP_INSERT``, ``OP_UPDATE``, and ``OP_DELETE``.

.. literalinclude:: ../examples/bulk/bulk6.c
   :language: c
   :caption: bulk6.c

The ``reply`` document is empty:

.. code-block:: none

  { }

Further Reading
---------------

See the `Driver Bulk API Spec <https://github.com/mongodb/specifications/blob/master/source/driver-bulk-update.rst>`_, which describes bulk write operations for all MongoDB drivers.

