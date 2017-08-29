:man_page: bson_visitor_t

bson_visitor_t
==============

Synopsis
--------

.. code-block:: c

  #include <bson.h>

  typedef struct {
     /* run before / after descending into a document */
     bool (*visit_before) (const bson_iter_t *iter, const char *key, void *data);
     bool (*visit_after) (const bson_iter_t *iter, const char *key, void *data);
     /* corrupt BSON, or unsupported type and visit_unsupported_type not set */
     void (*visit_corrupt) (const bson_iter_t *iter, void *data);
     /* normal bson field callbacks */
     bool (*visit_double) (const bson_iter_t *iter,
                           const char *key,
                           double v_double,
                           void *data);
     bool (*visit_utf8) (const bson_iter_t *iter,
                         const char *key,
                         size_t v_utf8_len,
                         const char *v_utf8,
                         void *data);
     bool (*visit_document) (const bson_iter_t *iter,
                             const char *key,
                             const bson_t *v_document,
                             void *data);
     bool (*visit_array) (const bson_iter_t *iter,
                          const char *key,
                          const bson_t *v_array,
                          void *data);
     bool (*visit_binary) (const bson_iter_t *iter,
                           const char *key,
                           bson_subtype_t v_subtype,
                           size_t v_binary_len,
                           const uint8_t *v_binary,
                           void *data);
     /* normal field with deprecated "Undefined" BSON type */
     bool (*visit_undefined) (const bson_iter_t *iter,
                              const char *key,
                              void *data);
     bool (*visit_oid) (const bson_iter_t *iter,
                        const char *key,
                        const bson_oid_t *v_oid,
                        void *data);
     bool (*visit_bool) (const bson_iter_t *iter,
                         const char *key,
                         bool v_bool,
                         void *data);
     bool (*visit_date_time) (const bson_iter_t *iter,
                              const char *key,
                              int64_t msec_since_epoch,
                              void *data);
     bool (*visit_null) (const bson_iter_t *iter, const char *key, void *data);
     bool (*visit_regex) (const bson_iter_t *iter,
                          const char *key,
                          const char *v_regex,
                          const char *v_options,
                          void *data);
     bool (*visit_dbpointer) (const bson_iter_t *iter,
                              const char *key,
                              size_t v_collection_len,
                              const char *v_collection,
                              const bson_oid_t *v_oid,
                              void *data);
     bool (*visit_code) (const bson_iter_t *iter,
                         const char *key,
                         size_t v_code_len,
                         const char *v_code,
                         void *data);
     bool (*visit_symbol) (const bson_iter_t *iter,
                           const char *key,
                           size_t v_symbol_len,
                           const char *v_symbol,
                           void *data);
     bool (*visit_codewscope) (const bson_iter_t *iter,
                               const char *key,
                               size_t v_code_len,
                               const char *v_code,
                               const bson_t *v_scope,
                               void *data);
     bool (*visit_int32) (const bson_iter_t *iter,
                          const char *key,
                          int32_t v_int32,
                          void *data);
     bool (*visit_timestamp) (const bson_iter_t *iter,
                              const char *key,
                              uint32_t v_timestamp,
                              uint32_t v_increment,
                              void *data);
     bool (*visit_int64) (const bson_iter_t *iter,
                          const char *key,
                          int64_t v_int64,
                          void *data);
     bool (*visit_maxkey) (const bson_iter_t *iter, const char *key, void *data);
     bool (*visit_minkey) (const bson_iter_t *iter, const char *key, void *data);
     /* if set, called instead of visit_corrupt when an apparently valid BSON
      * includes an unrecognized field type (reading future version of BSON) */
     void (*visit_unsupported_type) (const bson_iter_t *iter,
                                     const char *key,
                                     uint32_t type_code,
                                     void *data);
     bool (*visit_decimal128) (const bson_iter_t *iter,
                               const char *key,
                               const bson_decimal128_t *v_decimal128,
                               void *data);

     void *padding[7];
  } bson_visitor_t bson_visitor_t;

Description
-----------

The :symbol:`bson_visitor_t` structure provides a series of callbacks that can be called while iterating a BSON document. This may simplify the conversion of a :symbol:`bson_t` to a higher level language structure.

If the optional callback ``visit_unsupported_type`` is set, it is called instead of ``visit_corrupt`` in the specific case of an unrecognized field type. (Parsing is aborted in either case.) Use this callback to report an error like "unrecognized type" instead of simply "corrupt BSON". This future-proofs code that may use an older version of libbson to parse future BSON formats.

.. only:: html

  Functions
  ---------

  .. toctree::
    :titlesonly:
    :maxdepth: 1

Example
-------

.. code-block:: c

  #include <bson.h>
  #include <stdio.h>

  static bool
  my_visit_before (const bson_iter_t *iter, const char *key, void *data)
  {
     int *count = (int *) data;

     (*count)++;

     /* returning true stops further iteration of the document */

     return false;
  }

  static void
  count_fields (bson_t *doc)
  {
     bson_visitor_t visitor = {0};
     bson_iter_t iter;
     int count = 0;

     visitor.visit_before = my_visit_before;

     if (bson_iter_init (&iter, doc)) {
        bson_iter_visit_all (&iter, &visitor, &count);
     }

     printf ("Found %d fields.\n", count);
  }

