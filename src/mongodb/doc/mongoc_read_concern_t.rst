:man_page: mongoc_read_concern_t

mongoc_read_concern_t
=====================

Read Concern abstraction

Synopsis
--------

New in MongoDB 3.2.

The ``mongoc_read_concern_t`` allows clients to choose a level of isolation for their reads. The default, MONGOC_READ_CONCERN_LEVEL_LOCAL, is right for the great majority of applications.

You can specify a read concern on connection objects, database objects, or collection objects.

See `readConcern <https://docs.mongodb.org/master/reference/readConcern/>`_ on the MongoDB website for more information.

Read Concern is only sent to MongoDB when it has explicitly been set by :symbol:`mongoc_read_concern_set_level` to anything other than NULL.

.. _mongoc_read_concern_levels:

Read Concern Levels
-------------------

======================================  =========================== =====================
Macro                                   Description                 First MongoDB version
======================================  =========================== =====================
MONGOC_READ_CONCERN_LEVEL_LOCAL         Level "local", the default. 3.2
MONGOC_READ_CONCERN_LEVEL_MAJORITY      Level "majority".           3.2
MONGOC_READ_CONCERN_LEVEL_LINEARIZABLE  Level "linearizable".       3.4
======================================  =========================== =====================

For the sake of compatibility with future versions of MongoDB, :symbol:`mongoc_read_concern_set_level` allows any string, not just this list of known read concern levels.

See `Read Concern Levels <https://docs.mongodb.com/master/reference/read-concern/#read-concern-levels>`_ in the MongoDB manual for more information about the individual read concern levels.

.. only:: html

  Functions
  ---------

  .. toctree::
    :titlesonly:
    :maxdepth: 1

    mongoc_read_concern_append
    mongoc_read_concern_copy
    mongoc_read_concern_destroy
    mongoc_read_concern_get_level
    mongoc_read_concern_is_default
    mongoc_read_concern_new
    mongoc_read_concern_set_level

