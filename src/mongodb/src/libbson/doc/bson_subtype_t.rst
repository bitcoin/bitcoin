:man_page: bson_subtype_t

bson_subtype_t
==============

Binary Field Subtype

Synopsis
--------

.. code-block:: c

  #include <bson.h>


  typedef enum {
     BSON_SUBTYPE_BINARY = 0x00,
     BSON_SUBTYPE_FUNCTION = 0x01,
     BSON_SUBTYPE_BINARY_DEPRECATED = 0x02,
     BSON_SUBTYPE_UUID_DEPRECATED = 0x03,
     BSON_SUBTYPE_UUID = 0x04,
     BSON_SUBTYPE_MD5 = 0x05,
     BSON_SUBTYPE_USER = 0x80,
  } bson_subtype_t;

Description
-----------

This enumeration contains the various subtypes that may be used in a binary field. See `http://bsonspec.org <http://bsonspec.org>`_ for more information.

.. only:: html

  Functions
  ---------

  .. toctree::
    :titlesonly:
    :maxdepth: 1

Example
-------

.. code-block:: c

  bson_t doc = BSON_INITIALIZER;

  BSON_APPEND_BINARY (&doc, "binary", BSON_SUBTYPE_BINARY, data, data_len);

