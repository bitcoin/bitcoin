:man_page: mongoc_insert_flags_t

mongoc_insert_flags_t
=====================

Flags for insert operations

Synopsis
--------

.. code-block:: c

  typedef enum {
     MONGOC_INSERT_NONE = 0,
     MONGOC_INSERT_CONTINUE_ON_ERROR = 1 << 0,
  } mongoc_insert_flags_t;

  #define MONGOC_INSERT_NO_VALIDATE (1U << 31)

Description
-----------

These flags correspond to the MongoDB wire protocol. They may be bitwise or'd together. They may modify how an insert happens on the MongoDB server.

Flag Values
-----------

===============================  ======================================================================================================================================================================
MONGOC_INSERT_NONE               Specify no insert flags.                                                                                                                                              
MONGOC_INSERT_CONTINUE_ON_ERROR  Continue inserting documents from the insertion set even if one insert fails.                                                                                         
MONGOC_INSERT_NO_VALIDATE        Do not validate insertion documents before performing an insert. Validation can be expensive, so this can save some time if you know your documents are already valid.
===============================  ======================================================================================================================================================================

