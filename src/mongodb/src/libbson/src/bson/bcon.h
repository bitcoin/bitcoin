/*
 * @file bcon.h
 * @brief BCON (BSON C Object Notation) Declarations
 */

/*    Copyright 2009-2013 MongoDB, Inc.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#ifndef BCON_H_
#define BCON_H_

#include "bson.h"


BSON_BEGIN_DECLS


#define BCON_STACK_MAX 100

#define BCON_ENSURE_DECLARE(fun, type)                 \
   static BSON_INLINE type bcon_ensure_##fun (type _t) \
   {                                                   \
      return _t;                                       \
   }

#define BCON_ENSURE(fun, val) bcon_ensure_##fun (val)

#define BCON_ENSURE_STORAGE(fun, val) bcon_ensure_##fun (&(val))

BCON_ENSURE_DECLARE (const_char_ptr, const char *)
BCON_ENSURE_DECLARE (const_char_ptr_ptr, const char **)
BCON_ENSURE_DECLARE (double, double)
BCON_ENSURE_DECLARE (double_ptr, double *)
BCON_ENSURE_DECLARE (const_bson_ptr, const bson_t *)
BCON_ENSURE_DECLARE (bson_ptr, bson_t *)
BCON_ENSURE_DECLARE (subtype, bson_subtype_t)
BCON_ENSURE_DECLARE (subtype_ptr, bson_subtype_t *)
BCON_ENSURE_DECLARE (const_uint8_ptr, const uint8_t *)
BCON_ENSURE_DECLARE (const_uint8_ptr_ptr, const uint8_t **)
BCON_ENSURE_DECLARE (uint32, uint32_t)
BCON_ENSURE_DECLARE (uint32_ptr, uint32_t *)
BCON_ENSURE_DECLARE (const_oid_ptr, const bson_oid_t *)
BCON_ENSURE_DECLARE (const_oid_ptr_ptr, const bson_oid_t **)
BCON_ENSURE_DECLARE (int32, int32_t)
BCON_ENSURE_DECLARE (int32_ptr, int32_t *)
BCON_ENSURE_DECLARE (int64, int64_t)
BCON_ENSURE_DECLARE (int64_ptr, int64_t *)
BCON_ENSURE_DECLARE (const_decimal128_ptr, const bson_decimal128_t *)
BCON_ENSURE_DECLARE (bool, bool)
BCON_ENSURE_DECLARE (bool_ptr, bool *)
BCON_ENSURE_DECLARE (bson_type, bson_type_t)
BCON_ENSURE_DECLARE (bson_iter_ptr, bson_iter_t *)
BCON_ENSURE_DECLARE (const_bson_iter_ptr, const bson_iter_t *)

#define BCON_UTF8(_val) \
   BCON_MAGIC, BCON_TYPE_UTF8, BCON_ENSURE (const_char_ptr, (_val))
#define BCON_DOUBLE(_val) \
   BCON_MAGIC, BCON_TYPE_DOUBLE, BCON_ENSURE (double, (_val))
#define BCON_DOCUMENT(_val) \
   BCON_MAGIC, BCON_TYPE_DOCUMENT, BCON_ENSURE (const_bson_ptr, (_val))
#define BCON_ARRAY(_val) \
   BCON_MAGIC, BCON_TYPE_ARRAY, BCON_ENSURE (const_bson_ptr, (_val))
#define BCON_BIN(_subtype, _binary, _length)                     \
   BCON_MAGIC, BCON_TYPE_BIN, BCON_ENSURE (subtype, (_subtype)), \
      BCON_ENSURE (const_uint8_ptr, (_binary)),                  \
      BCON_ENSURE (uint32, (_length))
#define BCON_UNDEFINED BCON_MAGIC, BCON_TYPE_UNDEFINED
#define BCON_OID(_val) \
   BCON_MAGIC, BCON_TYPE_OID, BCON_ENSURE (const_oid_ptr, (_val))
#define BCON_BOOL(_val) BCON_MAGIC, BCON_TYPE_BOOL, BCON_ENSURE (bool, (_val))
#define BCON_DATE_TIME(_val) \
   BCON_MAGIC, BCON_TYPE_DATE_TIME, BCON_ENSURE (int64, (_val))
#define BCON_NULL BCON_MAGIC, BCON_TYPE_NULL
#define BCON_REGEX(_regex, _flags)                                      \
   BCON_MAGIC, BCON_TYPE_REGEX, BCON_ENSURE (const_char_ptr, (_regex)), \
      BCON_ENSURE (const_char_ptr, (_flags))
#define BCON_DBPOINTER(_collection, _oid)          \
   BCON_MAGIC, BCON_TYPE_DBPOINTER,                \
      BCON_ENSURE (const_char_ptr, (_collection)), \
      BCON_ENSURE (const_oid_ptr, (_oid))
#define BCON_CODE(_val) \
   BCON_MAGIC, BCON_TYPE_CODE, BCON_ENSURE (const_char_ptr, (_val))
#define BCON_SYMBOL(_val) \
   BCON_MAGIC, BCON_TYPE_SYMBOL, BCON_ENSURE (const_char_ptr, (_val))
#define BCON_CODEWSCOPE(_js, _scope)                                      \
   BCON_MAGIC, BCON_TYPE_CODEWSCOPE, BCON_ENSURE (const_char_ptr, (_js)), \
      BCON_ENSURE (const_bson_ptr, (_scope))
#define BCON_INT32(_val) \
   BCON_MAGIC, BCON_TYPE_INT32, BCON_ENSURE (int32, (_val))
#define BCON_TIMESTAMP(_timestamp, _increment)                         \
   BCON_MAGIC, BCON_TYPE_TIMESTAMP, BCON_ENSURE (int32, (_timestamp)), \
      BCON_ENSURE (int32, (_increment))
#define BCON_INT64(_val) \
   BCON_MAGIC, BCON_TYPE_INT64, BCON_ENSURE (int64, (_val))
#define BCON_DECIMAL128(_val) \
   BCON_MAGIC, BCON_TYPE_DECIMAL128, BCON_ENSURE (const_decimal128_ptr, (_val))
#define BCON_MAXKEY BCON_MAGIC, BCON_TYPE_MAXKEY
#define BCON_MINKEY BCON_MAGIC, BCON_TYPE_MINKEY
#define BCON(_val) \
   BCON_MAGIC, BCON_TYPE_BCON, BCON_ENSURE (const_bson_ptr, (_val))
#define BCON_ITER(_val) \
   BCON_MAGIC, BCON_TYPE_ITER, BCON_ENSURE (const_bson_iter_ptr, (_val))

#define BCONE_UTF8(_val) \
   BCONE_MAGIC, BCON_TYPE_UTF8, BCON_ENSURE_STORAGE (const_char_ptr_ptr, (_val))
#define BCONE_DOUBLE(_val) \
   BCONE_MAGIC, BCON_TYPE_DOUBLE, BCON_ENSURE_STORAGE (double_ptr, (_val))
#define BCONE_DOCUMENT(_val) \
   BCONE_MAGIC, BCON_TYPE_DOCUMENT, BCON_ENSURE_STORAGE (bson_ptr, (_val))
#define BCONE_ARRAY(_val) \
   BCONE_MAGIC, BCON_TYPE_ARRAY, BCON_ENSURE_STORAGE (bson_ptr, (_val))
#define BCONE_BIN(subtype, binary, length)                                   \
   BCONE_MAGIC, BCON_TYPE_BIN, BCON_ENSURE_STORAGE (subtype_ptr, (subtype)), \
      BCON_ENSURE_STORAGE (const_uint8_ptr_ptr, (binary)),                   \
      BCON_ENSURE_STORAGE (uint32_ptr, (length))
#define BCONE_UNDEFINED BCONE_MAGIC, BCON_TYPE_UNDEFINED
#define BCONE_OID(_val) \
   BCONE_MAGIC, BCON_TYPE_OID, BCON_ENSURE_STORAGE (const_oid_ptr_ptr, (_val))
#define BCONE_BOOL(_val) \
   BCONE_MAGIC, BCON_TYPE_BOOL, BCON_ENSURE_STORAGE (bool_ptr, (_val))
#define BCONE_DATE_TIME(_val) \
   BCONE_MAGIC, BCON_TYPE_DATE_TIME, BCON_ENSURE_STORAGE (int64_ptr, (_val))
#define BCONE_NULL BCONE_MAGIC, BCON_TYPE_NULL
#define BCONE_REGEX(_regex, _flags)                       \
   BCONE_MAGIC, BCON_TYPE_REGEX,                          \
      BCON_ENSURE_STORAGE (const_char_ptr_ptr, (_regex)), \
      BCON_ENSURE_STORAGE (const_char_ptr_ptr, (_flags))
#define BCONE_DBPOINTER(_collection, _oid)                     \
   BCONE_MAGIC, BCON_TYPE_DBPOINTER,                           \
      BCON_ENSURE_STORAGE (const_char_ptr_ptr, (_collection)), \
      BCON_ENSURE_STORAGE (const_oid_ptr_ptr, (_oid))
#define BCONE_CODE(_val) \
   BCONE_MAGIC, BCON_TYPE_CODE, BCON_ENSURE_STORAGE (const_char_ptr_ptr, (_val))
#define BCONE_SYMBOL(_val)        \
   BCONE_MAGIC, BCON_TYPE_SYMBOL, \
      BCON_ENSURE_STORAGE (const_char_ptr_ptr, (_val))
#define BCONE_CODEWSCOPE(_js, _scope)                  \
   BCONE_MAGIC, BCON_TYPE_CODEWSCOPE,                  \
      BCON_ENSURE_STORAGE (const_char_ptr_ptr, (_js)), \
      BCON_ENSURE_STORAGE (bson_ptr, (_scope))
#define BCONE_INT32(_val) \
   BCONE_MAGIC, BCON_TYPE_INT32, BCON_ENSURE_STORAGE (int32_ptr, (_val))
#define BCONE_TIMESTAMP(_timestamp, _increment)      \
   BCONE_MAGIC, BCON_TYPE_TIMESTAMP,                 \
      BCON_ENSURE_STORAGE (int32_ptr, (_timestamp)), \
      BCON_ENSURE_STORAGE (int32_ptr, (_increment))
#define BCONE_INT64(_val) \
   BCONE_MAGIC, BCON_TYPE_INT64, BCON_ENSURE_STORAGE (int64_ptr, (_val))
#define BCONE_DECIMAL128(_val)        \
   BCONE_MAGIC, BCON_TYPE_DECIMAL128, \
      BCON_ENSURE_STORAGE (const_decimal128_ptr, (_val))
#define BCONE_MAXKEY BCONE_MAGIC, BCON_TYPE_MAXKEY
#define BCONE_MINKEY BCONE_MAGIC, BCON_TYPE_MINKEY
#define BCONE_SKIP(_val) \
   BCONE_MAGIC, BCON_TYPE_SKIP, BCON_ENSURE (bson_type, (_val))
#define BCONE_ITER(_val) \
   BCONE_MAGIC, BCON_TYPE_ITER, BCON_ENSURE_STORAGE (bson_iter_ptr, (_val))

#define BCON_MAGIC bson_bcon_magic ()
#define BCONE_MAGIC bson_bcone_magic ()

typedef enum {
   BCON_TYPE_UTF8,
   BCON_TYPE_DOUBLE,
   BCON_TYPE_DOCUMENT,
   BCON_TYPE_ARRAY,
   BCON_TYPE_BIN,
   BCON_TYPE_UNDEFINED,
   BCON_TYPE_OID,
   BCON_TYPE_BOOL,
   BCON_TYPE_DATE_TIME,
   BCON_TYPE_NULL,
   BCON_TYPE_REGEX,
   BCON_TYPE_DBPOINTER,
   BCON_TYPE_CODE,
   BCON_TYPE_SYMBOL,
   BCON_TYPE_CODEWSCOPE,
   BCON_TYPE_INT32,
   BCON_TYPE_TIMESTAMP,
   BCON_TYPE_INT64,
   BCON_TYPE_DECIMAL128,
   BCON_TYPE_MAXKEY,
   BCON_TYPE_MINKEY,
   BCON_TYPE_BCON,
   BCON_TYPE_ARRAY_START,
   BCON_TYPE_ARRAY_END,
   BCON_TYPE_DOC_START,
   BCON_TYPE_DOC_END,
   BCON_TYPE_END,
   BCON_TYPE_RAW,
   BCON_TYPE_SKIP,
   BCON_TYPE_ITER,
   BCON_TYPE_ERROR,
} bcon_type_t;

typedef struct bcon_append_ctx_frame {
   int i;
   bool is_array;
   bson_t bson;
} bcon_append_ctx_frame_t;

typedef struct bcon_extract_ctx_frame {
   int i;
   bool is_array;
   bson_iter_t iter;
} bcon_extract_ctx_frame_t;

typedef struct _bcon_append_ctx_t {
   bcon_append_ctx_frame_t stack[BCON_STACK_MAX];
   int n;
} bcon_append_ctx_t;

typedef struct _bcon_extract_ctx_t {
   bcon_extract_ctx_frame_t stack[BCON_STACK_MAX];
   int n;
} bcon_extract_ctx_t;

BSON_EXPORT (void)
bcon_append (bson_t *bson, ...) BSON_GNUC_NULL_TERMINATED;
BSON_EXPORT (void)
bcon_append_ctx (bson_t *bson,
                 bcon_append_ctx_t *ctx,
                 ...) BSON_GNUC_NULL_TERMINATED;
BSON_EXPORT (void)
bcon_append_ctx_va (bson_t *bson, bcon_append_ctx_t *ctx, va_list *va);
BSON_EXPORT (void)
bcon_append_ctx_init (bcon_append_ctx_t *ctx);

BSON_EXPORT (void)
bcon_extract_ctx_init (bcon_extract_ctx_t *ctx);

BSON_EXPORT (void)
bcon_extract_ctx (bson_t *bson,
                  bcon_extract_ctx_t *ctx,
                  ...) BSON_GNUC_NULL_TERMINATED;

BSON_EXPORT (bool)
bcon_extract_ctx_va (bson_t *bson, bcon_extract_ctx_t *ctx, va_list *ap);

BSON_EXPORT (bool)
bcon_extract (bson_t *bson, ...) BSON_GNUC_NULL_TERMINATED;

BSON_EXPORT (bool)
bcon_extract_va (bson_t *bson,
                 bcon_extract_ctx_t *ctx,
                 ...) BSON_GNUC_NULL_TERMINATED;

BSON_EXPORT (bson_t *)
bcon_new (void *unused, ...) BSON_GNUC_NULL_TERMINATED;

/**
 * The bcon_..() functions are all declared with __attribute__((sentinel)).
 *
 * From GCC manual for "sentinel": "A valid NULL in this context is defined as
 * zero with any pointer type. If your system defines the NULL macro with an
 * integer type then you need to add an explicit cast."
 * Case in point: GCC on Solaris (at least)
 */
#define BCON_APPEND(_bson, ...) \
   bcon_append ((_bson), __VA_ARGS__, (void *) NULL)
#define BCON_APPEND_CTX(_bson, _ctx, ...) \
   bcon_append_ctx ((_bson), (_ctx), __VA_ARGS__, (void *) NULL)

#define BCON_EXTRACT(_bson, ...) \
   bcon_extract ((_bson), __VA_ARGS__, (void *) NULL)

#define BCON_EXTRACT_CTX(_bson, _ctx, ...) \
   bcon_extract ((_bson), (_ctx), __VA_ARGS__, (void *) NULL)

#define BCON_NEW(...) bcon_new (NULL, __VA_ARGS__, (void *) NULL)

BSON_EXPORT (const char *)
bson_bcon_magic (void) BSON_GNUC_CONST;
BSON_EXPORT (const char *)
bson_bcone_magic (void) BSON_GNUC_CONST;


BSON_END_DECLS


#endif
