:man_page: mongoc_query_flags_t

mongoc_query_flags_t
====================

Flags for query operations

Synopsis
--------

.. code-block:: c

  typedef enum {
     MONGOC_QUERY_NONE = 0,
     MONGOC_QUERY_TAILABLE_CURSOR = 1 << 1,
     MONGOC_QUERY_SLAVE_OK = 1 << 2,
     MONGOC_QUERY_OPLOG_REPLAY = 1 << 3,
     MONGOC_QUERY_NO_CURSOR_TIMEOUT = 1 << 4,
     MONGOC_QUERY_AWAIT_DATA = 1 << 5,
     MONGOC_QUERY_EXHAUST = 1 << 6,
     MONGOC_QUERY_PARTIAL = 1 << 7,
  } mongoc_query_flags_t;

Description
-----------

These flags correspond to the MongoDB wire protocol. They may be bitwise or'd together. They may modify how a query is performed in the MongoDB server.

Flag Values
-----------

==============================  =====================================================================================================================================================
MONGOC_QUERY_NONE               Specify no query flags.                                                                                                                              
MONGOC_QUERY_TAILABLE_CURSOR    Cursor will not be closed when the last data is retrieved. You can resume this cursor later.                                                         
MONGOC_QUERY_SLAVE_OK           Allow query of replica set secondaries.                                                                                                              
MONGOC_QUERY_OPLOG_REPLAY       Used internally by MongoDB.                                                                                                                          
MONGOC_QUERY_NO_CURSOR_TIMEOUT  The server normally times out an idle cursor after an inactivity period (10 minutes). This prevents that.                                            
MONGOC_QUERY_AWAIT_DATA         Use with MONGOC_QUERY_TAILABLE_CURSOR. Block rather than returning no data. After a period, time out.                                                
MONGOC_QUERY_EXHAUST            Stream the data down full blast in multiple "reply" packets. Faster when you are pulling down a lot of data and you know you want to retrieve it all.
MONGOC_QUERY_PARTIAL            Get partial results from mongos if some shards are down (instead of throwing an error).                                                              
==============================  =====================================================================================================================================================

