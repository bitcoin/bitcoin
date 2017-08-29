:man_page: mongoc_collection_t

mongoc_collection_t
===================

Synopsis
--------

.. code-block:: c

  #include <mongoc.h>

  typedef struct _mongoc_collection_t mongoc_collection_t;

``mongoc_collection_t`` provides access to a MongoDB collection.  This handle is useful for actions for most CRUD operations, I.e. insert, update, delete, find, etc.

Read Preferences and Write Concerns
-----------------------------------

Read preferences and write concerns are inherited from the parent client. They can be overridden by set_* commands if so desired.

.. only:: html

  Functions
  ---------

  .. toctree::
    :titlesonly:
    :maxdepth: 1

    mongoc_collection_aggregate
    mongoc_collection_command
    mongoc_collection_command_simple
    mongoc_collection_copy
    mongoc_collection_count
    mongoc_collection_count_with_opts
    mongoc_collection_create_bulk_operation
    mongoc_collection_create_index
    mongoc_collection_create_index_with_opts
    mongoc_collection_delete
    mongoc_collection_destroy
    mongoc_collection_drop
    mongoc_collection_drop_index
    mongoc_collection_drop_index_with_opts
    mongoc_collection_drop_with_opts
    mongoc_collection_ensure_index
    mongoc_collection_find
    mongoc_collection_find_and_modify
    mongoc_collection_find_and_modify_with_opts
    mongoc_collection_find_indexes
    mongoc_collection_find_with_opts
    mongoc_collection_get_last_error
    mongoc_collection_get_name
    mongoc_collection_get_read_concern
    mongoc_collection_get_read_prefs
    mongoc_collection_get_write_concern
    mongoc_collection_insert
    mongoc_collection_insert_bulk
    mongoc_collection_keys_to_index_string
    mongoc_collection_read_command_with_opts
    mongoc_collection_read_write_command_with_opts
    mongoc_collection_remove
    mongoc_collection_rename
    mongoc_collection_rename_with_opts
    mongoc_collection_save
    mongoc_collection_set_read_concern
    mongoc_collection_set_read_prefs
    mongoc_collection_set_write_concern
    mongoc_collection_stats
    mongoc_collection_update
    mongoc_collection_validate
    mongoc_collection_write_command_with_opts

