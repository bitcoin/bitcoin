:man_page: bson_t

bson_t
======

BSON Document Abstraction

Synopsis
--------

.. code-block:: c

  #include <bson.h>

  /**
   * bson_empty:
   * @b: a bson_t.
   *
   * Checks to see if @b is an empty BSON document. An empty BSON document is
   * a 5 byte document which contains the length (4 bytes) and a single NUL
   * byte indicating end of fields.
   */
  #define bson_empty(b) /* ... */

  /**
   * bson_empty0:
   *
   * Like bson_empty() but treats NULL the same as an empty bson_t document.
   */
  #define bson_empty0(b) /* ... */

  /**
   * bson_clear:
   *
   * Easily free a bson document and set it to NULL. Use like:
   *
   * bson_t *doc = bson_new();
   * bson_clear (&doc);
   * BSON_ASSERT (doc == NULL);
   */
  #define bson_clear(bptr) /* ... */

  /**
   * BSON_MAX_SIZE:
   *
   * The maximum size in bytes of a BSON document.
   */
  #define BSON_MAX_SIZE /* ... */

  #define BSON_APPEND_ARRAY(b, key, val) \
     bson_append_array (b, key, (int) strlen (key), val)

  #define BSON_APPEND_ARRAY_BEGIN(b, key, child) \
     bson_append_array_begin (b, key, (int) strlen (key), child)

  #define BSON_APPEND_BINARY(b, key, subtype, val, len) \
     bson_append_binary (b, key, (int) strlen (key), subtype, val, len)

  #define BSON_APPEND_BOOL(b, key, val) \
     bson_append_bool (b, key, (int) strlen (key), val)

  #define BSON_APPEND_CODE(b, key, val) \
     bson_append_code (b, key, (int) strlen (key), val)

  #define BSON_APPEND_CODE_WITH_SCOPE(b, key, val, scope) \
     bson_append_code_with_scope (b, key, (int) strlen (key), val, scope)

  #define BSON_APPEND_DBPOINTER(b, key, coll, oid) \
     bson_append_dbpointer (b, key, (int) strlen (key), coll, oid)

  #define BSON_APPEND_DOCUMENT_BEGIN(b, key, child) \
     bson_append_document_begin (b, key, (int) strlen (key), child)

  #define BSON_APPEND_DOUBLE(b, key, val) \
     bson_append_double (b, key, (int) strlen (key), val)

  #define BSON_APPEND_DOCUMENT(b, key, val) \
     bson_append_document (b, key, (int) strlen (key), val)

  #define BSON_APPEND_INT32(b, key, val) \
     bson_append_int32 (b, key, (int) strlen (key), val)

  #define BSON_APPEND_INT64(b, key, val) \
     bson_append_int64 (b, key, (int) strlen (key), val)

  #define BSON_APPEND_MINKEY(b, key) \
     bson_append_minkey (b, key, (int) strlen (key))

  #define BSON_APPEND_DECIMAL128(b, key, val) \
     bson_append_decimal128 (b, key, (int) strlen (key), val)

  #define BSON_APPEND_MAXKEY(b, key) \
     bson_append_maxkey (b, key, (int) strlen (key))

  #define BSON_APPEND_NULL(b, key) bson_append_null (b, key, (int) strlen (key))

  #define BSON_APPEND_OID(b, key, val) \
     bson_append_oid (b, key, (int) strlen (key), val)

  #define BSON_APPEND_REGEX(b, key, val, opt) \
     bson_append_regex (b, key, (int) strlen (key), val, opt)

  #define BSON_APPEND_UTF8(b, key, val) \
     bson_append_utf8 (b, key, (int) strlen (key), val, (int) strlen (val))

  #define BSON_APPEND_SYMBOL(b, key, val) \
     bson_append_symbol (b, key, (int) strlen (key), val, (int) strlen (val))

  #define BSON_APPEND_TIME_T(b, key, val) \
     bson_append_time_t (b, key, (int) strlen (key), val)

  #define BSON_APPEND_TIMEVAL(b, key, val) \
     bson_append_timeval (b, key, (int) strlen (key), val)

  #define BSON_APPEND_DATE_TIME(b, key, val) \
     bson_append_date_time (b, key, (int) strlen (key), val)

  #define BSON_APPEND_TIMESTAMP(b, key, val, inc) \
     bson_append_timestamp (b, key, (int) strlen (key), val, inc)

  #define BSON_APPEND_UNDEFINED(b, key) \
     bson_append_undefined (b, key, (int) strlen (key))

  #define BSON_APPEND_VALUE(b, key, val) \
     bson_append_value (b, key, (int) strlen (key), (val))

  BSON_ALIGNED_BEGIN (128)
  typedef struct {
     uint32_t flags;       /* Internal flags for the bson_t. */
     uint32_t len;         /* Length of BSON data. */
     uint8_t padding[120]; /* Padding for stack allocation. */
  } bson_t BSON_ALIGNED_END (128);

Description
-----------

The :symbol:`bson_t` structure represents a BSON document. This structure manages the underlying BSON encoded buffer. For mutable documents, it can append new data to the document.

Performance Notes
-----------------

The :symbol:`bson_t` structure attempts to use an inline allocation within the structure to speed up performance of small documents. When this internal buffer has been exhausted, a heap allocated buffer will be dynamically allocated. Therefore, it is essential to call :symbol:`bson_destroy()` on allocated documents.

.. only:: html

  Functions
  ---------

  .. toctree::
    :titlesonly:
    :maxdepth: 1

    bson_append_array
    bson_append_array_begin
    bson_append_array_end
    bson_append_binary
    bson_append_bool
    bson_append_code
    bson_append_code_with_scope
    bson_append_date_time
    bson_append_dbpointer
    bson_append_decimal128
    bson_append_document
    bson_append_document_begin
    bson_append_document_end
    bson_append_double
    bson_append_int32
    bson_append_int64
    bson_append_iter
    bson_append_maxkey
    bson_append_minkey
    bson_append_now_utc
    bson_append_null
    bson_append_oid
    bson_append_regex
    bson_append_symbol
    bson_append_time_t
    bson_append_timestamp
    bson_append_timeval
    bson_append_undefined
    bson_append_utf8
    bson_append_value
    bson_array_as_json
    bson_as_canonical_extended_json
    bson_as_json
    bson_as_relaxed_extended_json
    bson_compare
    bson_concat
    bson_copy
    bson_copy_to
    bson_copy_to_excluding
    bson_copy_to_excluding_noinit
    bson_count_keys
    bson_destroy
    bson_destroy_with_steal
    bson_equal
    bson_get_data
    bson_has_field
    bson_init
    bson_init_from_json
    bson_init_static
    bson_new
    bson_new_from_buffer
    bson_new_from_data
    bson_new_from_json
    bson_reinit
    bson_reserve_buffer
    bson_sized_new
    bson_steal
    bson_validate
    bson_validate_with_error

Example
-------

.. code-block:: c

  static void
  create_on_heap (void)
  {
     bson_t *b = bson_new ();

     BSON_APPEND_INT32 (b, "foo", 123);
     BSON_APPEND_UTF8 (b, "bar", "foo");
     BSON_APPEND_DOUBLE (b, "baz", 1.23f);

     bson_destroy (b);
  }

