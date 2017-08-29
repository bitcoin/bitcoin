:man_page: bson_type_t

bson_type_t
===========

BSON Type Enumeration

Synopsis
--------

.. code-block:: c

  #include <bson.h>

  typedef enum {
     BSON_TYPE_EOD = 0x00,
     BSON_TYPE_DOUBLE = 0x01,
     BSON_TYPE_UTF8 = 0x02,
     BSON_TYPE_DOCUMENT = 0x03,
     BSON_TYPE_ARRAY = 0x04,
     BSON_TYPE_BINARY = 0x05,
     BSON_TYPE_UNDEFINED = 0x06,
     BSON_TYPE_OID = 0x07,
     BSON_TYPE_BOOL = 0x08,
     BSON_TYPE_DATE_TIME = 0x09,
     BSON_TYPE_NULL = 0x0A,
     BSON_TYPE_REGEX = 0x0B,
     BSON_TYPE_DBPOINTER = 0x0C,
     BSON_TYPE_CODE = 0x0D,
     BSON_TYPE_SYMBOL = 0x0E,
     BSON_TYPE_CODEWSCOPE = 0x0F,
     BSON_TYPE_INT32 = 0x10,
     BSON_TYPE_TIMESTAMP = 0x11,
     BSON_TYPE_INT64 = 0x12,
     BSON_TYPE_MAXKEY = 0x7F,
     BSON_TYPE_MINKEY = 0xFF,
  } bson_type_t;

Description
-----------

The :symbol:`bson_type_t` enumeration contains all of the types from the `BSON Specification <http://bsonspec.org>`_. It can be used to determine the type of a field at runtime.

.. only:: html

  Functions
  ---------

  .. toctree::
    :titlesonly:
    :maxdepth: 1

Example
-------

.. code-block:: c

  bson_iter_t iter;

  if (bson_iter_init_find (&iter, doc, "foo") &&
      (BSON_TYPE_INT32 == bson_iter_type (&iter))) {
     printf ("'foo' is an int32.\n");
  }

