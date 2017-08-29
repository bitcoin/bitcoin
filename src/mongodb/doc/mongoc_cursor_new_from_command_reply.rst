:man_page: mongoc_cursor_new_from_command_reply

mongoc_cursor_new_from_command_reply()
======================================

Synopsis
--------

.. code-block:: c

  mongoc_cursor_t *
  mongoc_cursor_new_from_command_reply (mongoc_client_t *client,
                                        bson_t *reply,
                                        uint32_t server_id);

Parameters
----------

* ``client``: A :symbol:`mongoc_client_t`.
* ``reply``: The reply to a command, such as "aggregate", "find", or "listCollections", that returns a cursor document. The reply is destroyed by ``mongoc_cursor_new_from_command_reply`` and must not be accessed afterward.
* ``server_id``: The opaque id of the server used to execute the command.

Description
-----------

Some MongoDB commands return a "cursor" document. For example, given an "aggregate" command:

.. code-block:: none

  { "aggregate" : "collection", "pipeline" : [], "cursor" : {}}

The server replies:

.. code-block:: none

  {
     "cursor" : {
        "id" : 1234,
        "ns" : "db.collection",
        "firstBatch" : [ ]
     },
     "ok" : 1
  }

``mongoc_cursor_new_from_command_reply`` is a low-level function that initializes a :symbol:`mongoc_cursor_t` from such a reply.

When synthesizing a completed cursor response that has no more batches (i.e. with cursor id 0), set ``server_id`` to 0 as well.

Use this function only for building a language driver that wraps the C Driver. When writing applications in C, higher-level functions such as :symbol:`mongoc_collection_aggregate` are more appropriate, and ensure compatibility with a range of MongoDB versions.

Returns
-------

A :symbol:`mongoc_cursor_t`. On failure, the cursor's error is set. Check for failure with :symbol:`mongoc_cursor_error`.

