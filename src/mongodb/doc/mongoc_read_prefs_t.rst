:man_page: mongoc_read_prefs_t

mongoc_read_prefs_t
===================

A read preference abstraction

Synopsis
--------

:symbol:`mongoc_read_prefs_t` provides an abstraction on top of the MongoDB connection read prefences. It allows for hinting to the driver which nodes in a replica set should be accessed first.

You can specify a read preference mode on connection objects, database objects, collection objects, or per-operation.  Generally, it makes the most sense to stick with the global default, ``MONGOC_READ_PRIMARY``.  All of the other modes come with caveats that won't be covered in great detail here.

Read Modes
----------

===============================  ====================================================================================================================================================
MONGOC_READ_PRIMARY              Default mode. All operations read from the current replica set primary.                                                                             
MONGOC_READ_SECONDARY            All operations read from among the nearest secondary members of the replica set.                                                                    
MONGOC_READ_PRIMARY_PREFERRED    In most situations, operations read from the primary but if it is unavailable, operations read from secondary members.                              
MONGOC_READ_SECONDARY_PREFERRED  In most situations, operations read from among the nearest secondary members, but if no secondaries are available, operations read from the primary.
MONGOC_READ_NEAREST              Operations read from among the nearest members of the replica set, irrespective of the member's type.                                               
===============================  ====================================================================================================================================================

.. _mongoc-read-prefs-tag-sets:

Tag Sets
--------

Tag sets allow you to specify custom read preferences and write concerns so that your application can target operations to specific members.

Custom read preferences and write concerns evaluate tags sets in different ways: read preferences consider the value of a tag when selecting a member to read from. while write concerns ignore the value of a tag to when selecting a member except to consider whether or not the value is unique.

You can specify tag sets with the following read preference modes:

* primaryPreferred
* secondary
* secondaryPreferred
* nearest

Tags are not compatible with ``MONGOC_READ_PRIMARY`` and, in general, only apply when selecting a secondary member of a set for a read operation. However, the nearest read mode, when combined with a tag set will select the nearest member that matches the specified tag set, which may be a primary or secondary.


Tag sets are represented as a comma-separated list of colon-separated key-value
pairs when provided as a connection string, e.g. `dc:ny,rack:1`.

To specify a list of tag sets, using multiple readPreferenceTags, e.g.

.. code-block:: none

   readPreferenceTags=dc:ny,rack:1;readPreferenceTags=dc:ny;readPreferenceTags=

Note the empty value for the last one, which means match any secondary as a
last resort.

Order matters when using multiple readPreferenceTags.

Tag Sets can also be configured using :symbol:`mongoc_read_prefs_set_tags`.


All interfaces use the same member selection logic to choose the member to which to direct read operations, basing the choice on read preference mode and tag sets.

Max Staleness
-------------

When connected to replica set running MongoDB 3.4 or later, the driver estimates the staleness of each secondary based on lastWriteDate values provided in server isMaster responses.

Max Staleness is the maximum replication lag in seconds (wall clock time) that a secondary can suffer and still be eligible for reads. The default is ``MONGOC_NO_MAX_STALENESS``, which disables staleness checks. Otherwise, it must be a positive integer at least ``MONGOC_SMALLEST_MAX_STALENESS_SECONDS`` (90 seconds).

Max Staleness is also supported by sharded clusters of replica sets if all servers run MongoDB 3.4 or later.

.. only:: html

  Functions
  ---------

  .. toctree::
    :titlesonly:
    :maxdepth: 1

    mongoc_read_prefs_add_tag
    mongoc_read_prefs_copy
    mongoc_read_prefs_destroy
    mongoc_read_prefs_get_max_staleness_seconds
    mongoc_read_prefs_get_mode
    mongoc_read_prefs_get_tags
    mongoc_read_prefs_is_valid
    mongoc_read_prefs_new
    mongoc_read_prefs_set_max_staleness_seconds
    mongoc_read_prefs_set_mode
    mongoc_read_prefs_set_tags

