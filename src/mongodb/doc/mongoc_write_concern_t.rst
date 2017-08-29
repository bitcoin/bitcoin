:man_page: mongoc_write_concern_t

mongoc_write_concern_t
======================

Write Concern abstraction

Synopsis
--------

``mongoc_write_concern_t`` tells the driver what level of acknowledgment to await from the server. The default, MONGOC_WRITE_CONCERN_W_DEFAULT, is right for the great majority of applications.

You can specify a write concern on connection objects, database objects, collection objects, or per-operation. Data-modifying operations typically use the write concern of the object they operate on, and check the server response for a write concern error or write concern timeout. For example, :symbol:`mongoc_collection_drop_index` uses the collection's write concern, and a write concern error or timeout in the response is considered a failure.

Exceptions to this principle are the generic command functions:

* :symbol:`mongoc_client_command`
* :symbol:`mongoc_client_command_simple`
* :symbol:`mongoc_database_command`
* :symbol:`mongoc_database_command_simple`
* :symbol:`mongoc_collection_command`
* :symbol:`mongoc_collection_command_simple`

These generic command functions do not automatically apply a write concern, and they do not check the server response for a write concern error or write concern timeout.

See `Write Concern <http://docs.mongodb.org/manual/core/write-concern/>`_ on the MongoDB website for more information.

Write Concern Levels
--------------------

Set the write concern level with :symbol:`mongoc_write_concern_set_w`.

==========================================  ===============================================================================================================================================================================================================
MONGOC_WRITE_CONCERN_W_DEFAULT (1)          By default, writes block awaiting acknowledgment from MongoDB. Acknowledged write concern allows clients to catch network, duplicate key, and other errors.                                                    
MONGOC_WRITE_CONCERN_W_UNACKNOWLEDGED (0)   With this write concern, MongoDB does not acknowledge the receipt of write operation. Unacknowledged is similar to errors ignored; however, mongoc attempts to receive and handle network errors when possible.
MONGOC_WRITE_CONCERN_W_MAJORITY (majority)  Block until a write has been propagated to a majority of the nodes in the replica set.                                                                                                                         
n                                           Block until a write has been propagated to at least ``n`` nodes in the replica set.                                                                                                                            
==========================================  ===============================================================================================================================================================================================================

Deprecations
------------

The write concern ``MONGOC_WRITE_CONCERN_W_ERRORS_IGNORED`` (value -1) is a deprecated synonym for ``MONGOC_WRITE_CONCERN_W_UNACKNOWLEDGED`` (value 0), and will be removed in the next major release.

:symbol:`mongoc_write_concern_set_fsync` is deprecated.

.. only:: html

  Functions
  ---------

  .. toctree::
    :titlesonly:
    :maxdepth: 1

    mongoc_write_concern_append
    mongoc_write_concern_copy
    mongoc_write_concern_destroy
    mongoc_write_concern_get_fsync
    mongoc_write_concern_get_journal
    mongoc_write_concern_get_w
    mongoc_write_concern_get_wmajority
    mongoc_write_concern_get_wtag
    mongoc_write_concern_get_wtimeout
    mongoc_write_concern_is_acknowledged
    mongoc_write_concern_is_default
    mongoc_write_concern_is_valid
    mongoc_write_concern_journal_is_set
    mongoc_write_concern_new
    mongoc_write_concern_set_fsync
    mongoc_write_concern_set_journal
    mongoc_write_concern_set_w
    mongoc_write_concern_set_wmajority
    mongoc_write_concern_set_wtag
    mongoc_write_concern_set_wtimeout

