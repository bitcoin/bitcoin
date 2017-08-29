:man_page: bson_value_t

bson_value_t
============

BSON Boxed Container Type

Synopsis
--------

.. code-block:: c

  #include <bson.h>

  typedef struct _bson_value_t {
     bson_type_t value_type;
     union {
        bson_oid_t v_oid;
        int64_t v_int64;
        int32_t v_int32;
        int8_t v_int8;
        double v_double;
        bool v_bool;
        int64_t v_datetime;
        struct {
           uint32_t timestamp;
           uint32_t increment;
        } v_timestamp;
        struct {
           uint32_t len;
           char *str;
        } v_utf8;
        struct {
           uint32_t data_len;
           uint8_t *data;
        } v_doc;
        struct {
           uint32_t data_len;
           uint8_t *data;
           bson_subtype_t subtype;
        } v_binary;
        struct {
           char *regex;
           char *options;
        } v_regex;
        struct {
           char *collection;
           uint32_t collection_len;
           bson_oid_t oid;
        } v_dbpointer;
        struct {
           uint32_t code_len;
           char *code;
        } v_code;
        struct {
           uint32_t code_len;
           char *code;
           uint32_t scope_len;
           uint8_t *scope_data;
        } v_codewscope;
        struct {
           uint32_t len;
           char *symbol;
        } v_symbol;
     } value;
  } bson_value_t;

Description
-----------

The :symbol:`bson_value_t` structure is a boxed type for encapsulating a runtime determined type.

.. only:: html

  Functions
  ---------

  .. toctree::
    :titlesonly:
    :maxdepth: 1

    bson_value_copy
    bson_value_destroy

Example
-------

.. code-block:: c

  const bson_value_t *value;

  value = bson_iter_value (&iter);

  if (value->value_type == BSON_TYPE_INT32) {
     printf ("%d\n", value->value.v_int32);
  }

