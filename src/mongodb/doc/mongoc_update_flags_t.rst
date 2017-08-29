:man_page: mongoc_update_flags_t

mongoc_update_flags_t
=====================

Flags for update operations

Synopsis
--------

.. code-block:: c

  typedef enum {
     MONGOC_UPDATE_NONE = 0,
     MONGOC_UPDATE_UPSERT = 1 << 0,
     MONGOC_UPDATE_MULTI_UPDATE = 1 << 1,
  } mongoc_update_flags_t;

  #define MONGOC_UPDATE_NO_VALIDATE (1U << 31)

Description
-----------

These flags correspond to the MongoDB wire protocol. They may be bitwise or'd together. The allow for modifying the way an update is performed in the MongoDB server.

Flag Values
-----------

==========================  ========================================================================================================================================
MONGOC_UPDATE_NONE          No update flags set.                                                                                                                    
MONGOC_UPDATE_UPSERT        If an upsert should be performed.                                                                                                       
MONGOC_UPDATE_MULTI_UPDATE  If more than a single matching document should be updated. By default only the first document is updated.                               
MONGOC_UPDATE_NO_VALIDATE   Do not perform client side BSON validations when performing an update. This is useful if you already know your BSON documents are valid.
==========================  ========================================================================================================================================

