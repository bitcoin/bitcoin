:man_page: mongoc_read_mode_t

mongoc_read_mode_t
==================

Read Preference Modes

Synopsis
--------

.. code-block:: c

  typedef enum {
     MONGOC_READ_PRIMARY = (1 << 0),
     MONGOC_READ_SECONDARY = (1 << 1),
     MONGOC_READ_PRIMARY_PREFERRED = (1 << 2) | MONGOC_READ_PRIMARY,
     MONGOC_READ_SECONDARY_PREFERRED = (1 << 2) | MONGOC_READ_SECONDARY,
     MONGOC_READ_NEAREST = (1 << 3) | MONGOC_READ_SECONDARY,
  } mongoc_read_mode_t;

Description
-----------

This enum describes how reads should be dispatched. The default is MONGOC_READ_PRIMARY.

Please see the MongoDB website for a description of `Read Preferences <http://docs.mongodb.org/manual/core/read-preference/>`_.

