:man_page: mongoc_remove_flags_t

mongoc_remove_flags_t
=====================

Flags for deletion operations

Synopsis
--------

.. code-block:: c

  typedef enum {
     MONGOC_REMOVE_NONE = 0,
     MONGOC_REMOVE_SINGLE_REMOVE = 1 << 0,
  } mongoc_remove_flags_t;

Description
-----------

These flags correspond to the MongoDB wire protocol. They may be bitwise or'd together. They may change the number of documents that are removed during a remove command.

Flag Values
-----------

===========================  =================================================================
MONGOC_REMOVE_NONE           Specify no removal flags. All matching documents will be removed.
MONGOC_REMOVE_SINGLE_REMOVE  Only remove the first matching document from the selector.       
===========================  =================================================================

