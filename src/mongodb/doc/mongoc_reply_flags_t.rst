:man_page: mongoc_reply_flags_t

mongoc_reply_flags_t
====================

Flags from server replies

Synopsis
--------

.. code-block:: c

  typedef enum {
     MONGOC_REPLY_NONE = 0,
     MONGOC_REPLY_CURSOR_NOT_FOUND = 1 << 0,
     MONGOC_REPLY_QUERY_FAILURE = 1 << 1,
     MONGOC_REPLY_SHARD_CONFIG_STALE = 1 << 2,
     MONGOC_REPLY_AWAIT_CAPABLE = 1 << 3,
  } mongoc_reply_flags_t;

Description
-----------

These flags correspond to the wire protocol. They may be bitwise or'd together.

Flag Values
-----------

===============================  ==================================================================
MONGOC_REPLY_NONE                No flags set.                                                     
MONGOC_REPLY_CURSOR_NOT_FOUND    No matching cursor was found on the server.                       
MONGOC_REPLY_QUERY_FAILURE       The query failed or was invalid. Error document has been provided.
MONGOC_REPLY_SHARD_CONFIG_STALE  Shard config is stale.                                            
MONGOC_REPLY_AWAIT_CAPABLE       If the returned cursor is capable of MONGOC_QUERY_AWAIT_DATA.     
===============================  ==================================================================

