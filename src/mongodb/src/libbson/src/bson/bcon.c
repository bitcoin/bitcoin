/*
 * @file bcon.c
 * @brief BCON (BSON C Object Notation) Implementation
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


#include <stdio.h>

#include "bcon.h"
#include "bson-config.h"

/* These stack manipulation macros are used to manage append recursion in
 * bcon_append_ctx_va().  They take care of some awkward dereference rules (the
 * real bson object isn't in the stack, but accessed by pointer) and add in run
 * time asserts to make sure we don't blow the stack in either direction */

#define STACK_ELE(_delta, _name) (ctx->stack[(_delta) + ctx->n]._name)

#define STACK_BSON(_delta) \
   (((_delta) + ctx->n) == 0 ? bson : &STACK_ELE (_delta, bson))

#define STACK_ITER(_delta) \
   (((_delta) + ctx->n) == 0 ? &root_iter : &STACK_ELE (_delta, iter))

#define STACK_BSON_PARENT STACK_BSON (-1)
#define STACK_BSON_CHILD STACK_BSON (0)

#define STACK_ITER_PARENT STACK_ITER (-1)
#define STACK_ITER_CHILD STACK_ITER (0)

#define STACK_I STACK_ELE (0, i)
#define STACK_IS_ARRAY STACK_ELE (0, is_array)

#define STACK_PUSH_ARRAY(statement)                \
   do {                                            \
      BSON_ASSERT (ctx->n < (BCON_STACK_MAX - 1)); \
      ctx->n++;                                    \
      STACK_I = 0;                                 \
      STACK_IS_ARRAY = 1;                          \
      statement;                                   \
   } while (0)

#define STACK_PUSH_DOC(statement)                  \
   do {                                            \
      BSON_ASSERT (ctx->n < (BCON_STACK_MAX - 1)); \
      ctx->n++;                                    \
      STACK_IS_ARRAY = 0;                          \
      statement;                                   \
   } while (0)

#define STACK_POP_ARRAY(statement)  \
   do {                             \
      BSON_ASSERT (STACK_IS_ARRAY); \
      BSON_ASSERT (ctx->n != 0);    \
      statement;                    \
      ctx->n--;                     \
   } while (0)

#define STACK_POP_DOC(statement)     \
   do {                              \
      BSON_ASSERT (!STACK_IS_ARRAY); \
      BSON_ASSERT (ctx->n != 0);     \
      statement;                     \
      ctx->n--;                      \
   } while (0)

/* This is a landing pad union for all of the types we can process with bcon.
 * We need actual storage for this to capture the return value of va_arg, which
 * takes multiple calls to get everything we need for some complex types */
typedef union bcon_append {
   char *UTF8;
   double DOUBLE;
   bson_t *DOCUMENT;
   bson_t *ARRAY;
   bson_t *BCON;

   struct {
      bson_subtype_t subtype;
      uint8_t *binary;
      uint32_t length;
   } BIN;

   bson_oid_t *OID;
   bool BOOL;
   int64_t DATE_TIME;

   struct {
      char *regex;
      char *flags;
   } REGEX;

   struct {
      char *collection;
      bson_oid_t *oid;
   } DBPOINTER;

   const char *CODE;

   char *SYMBOL;

   struct {
      const char *js;
      bson_t *scope;
   } CODEWSCOPE;

   int32_t INT32;

   struct {
      uint32_t timestamp;
      uint32_t increment;
   } TIMESTAMP;

   int64_t INT64;
   bson_decimal128_t *DECIMAL128;
   const bson_iter_t *ITER;
} bcon_append_t;

/* same as bcon_append_t.  Some extra symbols and varying types that handle the
 * differences between bson_append and bson_iter */
typedef union bcon_extract {
   bson_type_t TYPE;
   bson_iter_t *ITER;
   const char *key;
   const char **UTF8;
   double *DOUBLE;
   bson_t *DOCUMENT;
   bson_t *ARRAY;

   struct {
      bson_subtype_t *subtype;
      const uint8_t **binary;
      uint32_t *length;
   } BIN;

   const bson_oid_t **OID;
   bool *BOOL;
   int64_t *DATE_TIME;

   struct {
      const char **regex;
      const char **flags;
   } REGEX;

   struct {
      const char **collection;
      const bson_oid_t **oid;
   } DBPOINTER;

   const char **CODE;

   const char **SYMBOL;

   struct {
      const char **js;
      bson_t *scope;
   } CODEWSCOPE;

   int32_t *INT32;

   struct {
      uint32_t *timestamp;
      uint32_t *increment;
   } TIMESTAMP;

   int64_t *INT64;
   bson_decimal128_t *DECIMAL128;
} bcon_extract_t;

static const char *gBconMagic = "BCON_MAGIC";
static const char *gBconeMagic = "BCONE_MAGIC";

const char *
bson_bcon_magic (void)
{
   return gBconMagic;
}


const char *
bson_bcone_magic (void)
{
   return gBconeMagic;
}

static void
_noop (void)
{
}

/* appends val to the passed bson object.  Meant to be a super simple dispatch
 * table */
static void
_bcon_append_single (bson_t *bson,
                     bcon_type_t type,
                     const char *key,
                     bcon_append_t *val)
{
   switch ((int) type) {
   case BCON_TYPE_UTF8:
      bson_append_utf8 (bson, key, -1, val->UTF8, -1);
      break;
   case BCON_TYPE_DOUBLE:
      bson_append_double (bson, key, -1, val->DOUBLE);
      break;
   case BCON_TYPE_BIN: {
      bson_append_binary (
         bson, key, -1, val->BIN.subtype, val->BIN.binary, val->BIN.length);
      break;
   }
   case BCON_TYPE_UNDEFINED:
      bson_append_undefined (bson, key, -1);
      break;
   case BCON_TYPE_OID:
      bson_append_oid (bson, key, -1, val->OID);
      break;
   case BCON_TYPE_BOOL:
      bson_append_bool (bson, key, -1, (bool) val->BOOL);
      break;
   case BCON_TYPE_DATE_TIME:
      bson_append_date_time (bson, key, -1, val->DATE_TIME);
      break;
   case BCON_TYPE_NULL:
      bson_append_null (bson, key, -1);
      break;
   case BCON_TYPE_REGEX: {
      bson_append_regex (bson, key, -1, val->REGEX.regex, val->REGEX.flags);
      break;
   }
   case BCON_TYPE_DBPOINTER: {
      bson_append_dbpointer (
         bson, key, -1, val->DBPOINTER.collection, val->DBPOINTER.oid);
      break;
   }
   case BCON_TYPE_CODE:
      bson_append_code (bson, key, -1, val->CODE);
      break;
   case BCON_TYPE_SYMBOL:
      bson_append_symbol (bson, key, -1, val->SYMBOL, -1);
      break;
   case BCON_TYPE_CODEWSCOPE:
      bson_append_code_with_scope (
         bson, key, -1, val->CODEWSCOPE.js, val->CODEWSCOPE.scope);
      break;
   case BCON_TYPE_INT32:
      bson_append_int32 (bson, key, -1, val->INT32);
      break;
   case BCON_TYPE_TIMESTAMP: {
      bson_append_timestamp (
         bson, key, -1, val->TIMESTAMP.timestamp, val->TIMESTAMP.increment);
      break;
   }
   case BCON_TYPE_INT64:
      bson_append_int64 (bson, key, -1, val->INT64);
      break;
   case BCON_TYPE_DECIMAL128:
      bson_append_decimal128 (bson, key, -1, val->DECIMAL128);
      break;
   case BCON_TYPE_MAXKEY:
      bson_append_maxkey (bson, key, -1);
      break;
   case BCON_TYPE_MINKEY:
      bson_append_minkey (bson, key, -1);
      break;
   case BCON_TYPE_ARRAY: {
      bson_append_array (bson, key, -1, val->ARRAY);
      break;
   }
   case BCON_TYPE_DOCUMENT: {
      bson_append_document (bson, key, -1, val->DOCUMENT);
      break;
   }
   case BCON_TYPE_ITER:
      bson_append_iter (bson, key, -1, val->ITER);
      break;
   default:
      BSON_ASSERT (0);
      break;
   }
}

#define CHECK_TYPE(_type)                     \
   do {                                       \
      if (bson_iter_type (iter) != (_type)) { \
         return false;                        \
      }                                       \
   } while (0)

/* extracts the value under the iterator and writes it to val.  returns false
 * if the iterator type doesn't match the token type.
 *
 * There are two magic tokens:
 *
 * BCONE_SKIP -
 *    Let's us verify that a key has a type, without caring about its value.
 *    This allows for wider declarative BSON verification
 *
 * BCONE_ITER -
 *    Returns the underlying iterator.  This could allow for more complicated,
 *    procedural verification (if a parameter could have multiple types).
 * */
static bool
_bcon_extract_single (const bson_iter_t *iter,
                      bcon_type_t type,
                      bcon_extract_t *val)
{
   switch ((int) type) {
   case BCON_TYPE_UTF8:
      CHECK_TYPE (BSON_TYPE_UTF8);
      *val->UTF8 = bson_iter_utf8 (iter, NULL);
      break;
   case BCON_TYPE_DOUBLE:
      CHECK_TYPE (BSON_TYPE_DOUBLE);
      *val->DOUBLE = bson_iter_double (iter);
      break;
   case BCON_TYPE_BIN:
      CHECK_TYPE (BSON_TYPE_BINARY);
      bson_iter_binary (
         iter, val->BIN.subtype, val->BIN.length, val->BIN.binary);
      break;
   case BCON_TYPE_UNDEFINED:
      CHECK_TYPE (BSON_TYPE_UNDEFINED);
      break;
   case BCON_TYPE_OID:
      CHECK_TYPE (BSON_TYPE_OID);
      *val->OID = bson_iter_oid (iter);
      break;
   case BCON_TYPE_BOOL:
      CHECK_TYPE (BSON_TYPE_BOOL);
      *val->BOOL = bson_iter_bool (iter);
      break;
   case BCON_TYPE_DATE_TIME:
      CHECK_TYPE (BSON_TYPE_DATE_TIME);
      *val->DATE_TIME = bson_iter_date_time (iter);
      break;
   case BCON_TYPE_NULL:
      CHECK_TYPE (BSON_TYPE_NULL);
      break;
   case BCON_TYPE_REGEX:
      CHECK_TYPE (BSON_TYPE_REGEX);
      *val->REGEX.regex = bson_iter_regex (iter, val->REGEX.flags);

      break;
   case BCON_TYPE_DBPOINTER:
      CHECK_TYPE (BSON_TYPE_DBPOINTER);
      bson_iter_dbpointer (
         iter, NULL, val->DBPOINTER.collection, val->DBPOINTER.oid);
      break;
   case BCON_TYPE_CODE:
      CHECK_TYPE (BSON_TYPE_CODE);
      *val->CODE = bson_iter_code (iter, NULL);
      break;
   case BCON_TYPE_SYMBOL:
      CHECK_TYPE (BSON_TYPE_SYMBOL);
      *val->SYMBOL = bson_iter_symbol (iter, NULL);
      break;
   case BCON_TYPE_CODEWSCOPE: {
      const uint8_t *buf;
      uint32_t len;

      CHECK_TYPE (BSON_TYPE_CODEWSCOPE);

      *val->CODEWSCOPE.js = bson_iter_codewscope (iter, NULL, &len, &buf);

      bson_init_static (val->CODEWSCOPE.scope, buf, len);
      break;
   }
   case BCON_TYPE_INT32:
      CHECK_TYPE (BSON_TYPE_INT32);
      *val->INT32 = bson_iter_int32 (iter);
      break;
   case BCON_TYPE_TIMESTAMP:
      CHECK_TYPE (BSON_TYPE_TIMESTAMP);
      bson_iter_timestamp (
         iter, val->TIMESTAMP.timestamp, val->TIMESTAMP.increment);
      break;
   case BCON_TYPE_INT64:
      CHECK_TYPE (BSON_TYPE_INT64);
      *val->INT64 = bson_iter_int64 (iter);
      break;
   case BCON_TYPE_DECIMAL128:
      CHECK_TYPE (BSON_TYPE_DECIMAL128);
      bson_iter_decimal128 (iter, val->DECIMAL128);
      break;
   case BCON_TYPE_MAXKEY:
      CHECK_TYPE (BSON_TYPE_MAXKEY);
      break;
   case BCON_TYPE_MINKEY:
      CHECK_TYPE (BSON_TYPE_MINKEY);
      break;
   case BCON_TYPE_ARRAY: {
      const uint8_t *buf;
      uint32_t len;

      CHECK_TYPE (BSON_TYPE_ARRAY);

      bson_iter_array (iter, &len, &buf);

      bson_init_static (val->ARRAY, buf, len);
      break;
   }
   case BCON_TYPE_DOCUMENT: {
      const uint8_t *buf;
      uint32_t len;

      CHECK_TYPE (BSON_TYPE_DOCUMENT);

      bson_iter_document (iter, &len, &buf);

      bson_init_static (val->DOCUMENT, buf, len);
      break;
   }
   case BCON_TYPE_SKIP:
      CHECK_TYPE (val->TYPE);
      break;
   case BCON_TYPE_ITER:
      memcpy (val->ITER, iter, sizeof *iter);
      break;
   default:
      BSON_ASSERT (0);
      break;
   }

   return true;
}

/* Consumes ap, storing output values into u and returning the type of the
 * captured token.
 *
 * The basic workflow goes like this:
 *
 * 1. Look at the current arg.  It will be a char *
 *    a. If it's a NULL, we're done processing.
 *    b. If it's BCON_MAGIC (a symbol with storage in this module)
 *       I. The next token is the type
 *       II. The type specifies how many args to eat and their types
 *    c. Otherwise it's either recursion related or a raw string
 *       I. If the first byte is '{', '}', '[', or ']' pass back an
 *          appropriate recursion token
 *       II. If not, just call it a UTF8 token and pass that back
 */
static bcon_type_t
_bcon_append_tokenize (va_list *ap, bcon_append_t *u)
{
   char *mark;
   bcon_type_t type;

   mark = va_arg (*ap, char *);

   BSON_ASSERT (mark != BCONE_MAGIC);

   if (mark == NULL) {
      type = BCON_TYPE_END;
   } else if (mark == BCON_MAGIC) {
      type = va_arg (*ap, bcon_type_t);

      switch ((int) type) {
      case BCON_TYPE_UTF8:
         u->UTF8 = va_arg (*ap, char *);
         break;
      case BCON_TYPE_DOUBLE:
         u->DOUBLE = va_arg (*ap, double);
         break;
      case BCON_TYPE_DOCUMENT:
         u->DOCUMENT = va_arg (*ap, bson_t *);
         break;
      case BCON_TYPE_ARRAY:
         u->ARRAY = va_arg (*ap, bson_t *);
         break;
      case BCON_TYPE_BIN:
         u->BIN.subtype = va_arg (*ap, bson_subtype_t);
         u->BIN.binary = va_arg (*ap, uint8_t *);
         u->BIN.length = va_arg (*ap, uint32_t);
         break;
      case BCON_TYPE_UNDEFINED:
         break;
      case BCON_TYPE_OID:
         u->OID = va_arg (*ap, bson_oid_t *);
         break;
      case BCON_TYPE_BOOL:
         u->BOOL = va_arg (*ap, int);
         break;
      case BCON_TYPE_DATE_TIME:
         u->DATE_TIME = va_arg (*ap, int64_t);
         break;
      case BCON_TYPE_NULL:
         break;
      case BCON_TYPE_REGEX:
         u->REGEX.regex = va_arg (*ap, char *);
         u->REGEX.flags = va_arg (*ap, char *);
         break;
      case BCON_TYPE_DBPOINTER:
         u->DBPOINTER.collection = va_arg (*ap, char *);
         u->DBPOINTER.oid = va_arg (*ap, bson_oid_t *);
         break;
      case BCON_TYPE_CODE:
         u->CODE = va_arg (*ap, char *);
         break;
      case BCON_TYPE_SYMBOL:
         u->SYMBOL = va_arg (*ap, char *);
         break;
      case BCON_TYPE_CODEWSCOPE:
         u->CODEWSCOPE.js = va_arg (*ap, char *);
         u->CODEWSCOPE.scope = va_arg (*ap, bson_t *);
         break;
      case BCON_TYPE_INT32:
         u->INT32 = va_arg (*ap, int32_t);
         break;
      case BCON_TYPE_TIMESTAMP:
         u->TIMESTAMP.timestamp = va_arg (*ap, uint32_t);
         u->TIMESTAMP.increment = va_arg (*ap, uint32_t);
         break;
      case BCON_TYPE_INT64:
         u->INT64 = va_arg (*ap, int64_t);
         break;
      case BCON_TYPE_DECIMAL128:
         u->DECIMAL128 = va_arg (*ap, bson_decimal128_t *);
         break;
      case BCON_TYPE_MAXKEY:
         break;
      case BCON_TYPE_MINKEY:
         break;
      case BCON_TYPE_BCON:
         u->BCON = va_arg (*ap, bson_t *);
         break;
      case BCON_TYPE_ITER:
         u->ITER = va_arg (*ap, const bson_iter_t *);
         break;
      default:
         BSON_ASSERT (0);
         break;
      }
   } else {
      switch (mark[0]) {
      case '{':
         type = BCON_TYPE_DOC_START;
         break;
      case '}':
         type = BCON_TYPE_DOC_END;
         break;
      case '[':
         type = BCON_TYPE_ARRAY_START;
         break;
      case ']':
         type = BCON_TYPE_ARRAY_END;
         break;

      default:
         type = BCON_TYPE_UTF8;
         u->UTF8 = mark;
         break;
      }
   }

   return type;
}


/* Consumes ap, storing output values into u and returning the type of the
 * captured token.
 *
 * The basic workflow goes like this:
 *
 * 1. Look at the current arg.  It will be a char *
 *    a. If it's a NULL, we're done processing.
 *    b. If it's BCONE_MAGIC (a symbol with storage in this module)
 *       I. The next token is the type
 *       II. The type specifies how many args to eat and their types
 *    c. Otherwise it's either recursion related or a raw string
 *       I. If the first byte is '{', '}', '[', or ']' pass back an
 *          appropriate recursion token
 *       II. If not, just call it a UTF8 token and pass that back
 */
static bcon_type_t
_bcon_extract_tokenize (va_list *ap, bcon_extract_t *u)
{
   char *mark;
   bcon_type_t type;

   mark = va_arg (*ap, char *);

   BSON_ASSERT (mark != BCON_MAGIC);

   if (mark == NULL) {
      type = BCON_TYPE_END;
   } else if (mark == BCONE_MAGIC) {
      type = va_arg (*ap, bcon_type_t);

      switch ((int) type) {
      case BCON_TYPE_UTF8:
         u->UTF8 = va_arg (*ap, const char **);
         break;
      case BCON_TYPE_DOUBLE:
         u->DOUBLE = va_arg (*ap, double *);
         break;
      case BCON_TYPE_DOCUMENT:
         u->DOCUMENT = va_arg (*ap, bson_t *);
         break;
      case BCON_TYPE_ARRAY:
         u->ARRAY = va_arg (*ap, bson_t *);
         break;
      case BCON_TYPE_BIN:
         u->BIN.subtype = va_arg (*ap, bson_subtype_t *);
         u->BIN.binary = va_arg (*ap, const uint8_t **);
         u->BIN.length = va_arg (*ap, uint32_t *);
         break;
      case BCON_TYPE_UNDEFINED:
         break;
      case BCON_TYPE_OID:
         u->OID = va_arg (*ap, const bson_oid_t **);
         break;
      case BCON_TYPE_BOOL:
         u->BOOL = va_arg (*ap, bool *);
         break;
      case BCON_TYPE_DATE_TIME:
         u->DATE_TIME = va_arg (*ap, int64_t *);
         break;
      case BCON_TYPE_NULL:
         break;
      case BCON_TYPE_REGEX:
         u->REGEX.regex = va_arg (*ap, const char **);
         u->REGEX.flags = va_arg (*ap, const char **);
         break;
      case BCON_TYPE_DBPOINTER:
         u->DBPOINTER.collection = va_arg (*ap, const char **);
         u->DBPOINTER.oid = va_arg (*ap, const bson_oid_t **);
         break;
      case BCON_TYPE_CODE:
         u->CODE = va_arg (*ap, const char **);
         break;
      case BCON_TYPE_SYMBOL:
         u->SYMBOL = va_arg (*ap, const char **);
         break;
      case BCON_TYPE_CODEWSCOPE:
         u->CODEWSCOPE.js = va_arg (*ap, const char **);
         u->CODEWSCOPE.scope = va_arg (*ap, bson_t *);
         break;
      case BCON_TYPE_INT32:
         u->INT32 = va_arg (*ap, int32_t *);
         break;
      case BCON_TYPE_TIMESTAMP:
         u->TIMESTAMP.timestamp = va_arg (*ap, uint32_t *);
         u->TIMESTAMP.increment = va_arg (*ap, uint32_t *);
         break;
      case BCON_TYPE_INT64:
         u->INT64 = va_arg (*ap, int64_t *);
         break;
      case BCON_TYPE_DECIMAL128:
         u->DECIMAL128 = va_arg (*ap, bson_decimal128_t *);
         break;
      case BCON_TYPE_MAXKEY:
         break;
      case BCON_TYPE_MINKEY:
         break;
      case BCON_TYPE_SKIP:
         u->TYPE = va_arg (*ap, bson_type_t);
         break;
      case BCON_TYPE_ITER:
         u->ITER = va_arg (*ap, bson_iter_t *);
         break;
      default:
         BSON_ASSERT (0);
         break;
      }
   } else {
      switch (mark[0]) {
      case '{':
         type = BCON_TYPE_DOC_START;
         break;
      case '}':
         type = BCON_TYPE_DOC_END;
         break;
      case '[':
         type = BCON_TYPE_ARRAY_START;
         break;
      case ']':
         type = BCON_TYPE_ARRAY_END;
         break;

      default:
         type = BCON_TYPE_RAW;
         u->key = mark;
         break;
      }
   }

   return type;
}


/* This trivial utility function is useful for concatenating a bson object onto
 * the end of another, ignoring the keys from the source bson object and
 * continuing to use and increment the keys from the source.  It's only useful
 * when called from bcon_append_ctx_va */
static void
_bson_concat_array (bson_t *dest, const bson_t *src, bcon_append_ctx_t *ctx)
{
   bson_iter_t iter;
   const char *key;
   char i_str[16];
   bool r;

   r = bson_iter_init (&iter, src);

   if (!r) {
      fprintf (stderr, "Invalid BSON document, possible memory coruption.\n");
      return;
   }

   STACK_I--;

   while (bson_iter_next (&iter)) {
      bson_uint32_to_string (STACK_I, &key, i_str, sizeof i_str);
      STACK_I++;

      bson_append_iter (dest, key, -1, &iter);
   }
}


/* Append_ctx_va consumes the va_list until NULL is found, appending into bson
 * as tokens are found.  It can receive or return an in-progress bson object
 * via the ctx param.  It can also operate on the middle of a va_list, and so
 * can be wrapped inside of another varargs function.
 *
 * Note that passing in a va_list that isn't perferectly formatted for BCON
 * ingestion will almost certainly result in undefined behavior
 *
 * The workflow relies on the passed ctx object, which holds a stack of bson
 * objects, along with metadata (if the emedded layer is an array, and which
 * element it is on if so).  We iterate, generating tokens from the va_list,
 * until we reach an END token.  If any errors occur, we just blow up (the
 * var_args stuff is already incredibly fragile to mistakes, and we have no way
 * of introspecting, so just don't screw it up).
 *
 * There are also a few STACK_* macros in here which manimpulate ctx that are
 * defined up top.
 * */
void
bcon_append_ctx_va (bson_t *bson, bcon_append_ctx_t *ctx, va_list *ap)
{
   bcon_type_t type;
   const char *key;
   char i_str[16];

   bcon_append_t u = {0};

   while (1) {
      if (STACK_IS_ARRAY) {
         bson_uint32_to_string (STACK_I, &key, i_str, sizeof i_str);
         STACK_I++;
      } else {
         type = _bcon_append_tokenize (ap, &u);

         if (type == BCON_TYPE_END) {
            return;
         }

         if (type == BCON_TYPE_DOC_END) {
            STACK_POP_DOC (
               bson_append_document_end (STACK_BSON_PARENT, STACK_BSON_CHILD));
            continue;
         }

         if (type == BCON_TYPE_BCON) {
            bson_concat (STACK_BSON_CHILD, u.BCON);
            continue;
         }

         BSON_ASSERT (type == BCON_TYPE_UTF8);

         key = u.UTF8;
      }

      type = _bcon_append_tokenize (ap, &u);
      BSON_ASSERT (type != BCON_TYPE_END);

      switch ((int) type) {
      case BCON_TYPE_BCON:
         BSON_ASSERT (STACK_IS_ARRAY);
         _bson_concat_array (STACK_BSON_CHILD, u.BCON, ctx);

         break;
      case BCON_TYPE_DOC_START:
         STACK_PUSH_DOC (bson_append_document_begin (
            STACK_BSON_PARENT, key, -1, STACK_BSON_CHILD));
         break;
      case BCON_TYPE_DOC_END:
         STACK_POP_DOC (
            bson_append_document_end (STACK_BSON_PARENT, STACK_BSON_CHILD));
         break;
      case BCON_TYPE_ARRAY_START:
         STACK_PUSH_ARRAY (bson_append_array_begin (
            STACK_BSON_PARENT, key, -1, STACK_BSON_CHILD));
         break;
      case BCON_TYPE_ARRAY_END:
         STACK_POP_ARRAY (
            bson_append_array_end (STACK_BSON_PARENT, STACK_BSON_CHILD));
         break;
      default:
         _bcon_append_single (STACK_BSON_CHILD, type, key, &u);

         break;
      }
   }
}


/* extract_ctx_va consumes the va_list until NULL is found, extracting values
 * as tokens are found.  It can receive or return an in-progress bson object
 * via the ctx param.  It can also operate on the middle of a va_list, and so
 * can be wrapped inside of another varargs function.
 *
 * Note that passing in a va_list that isn't perferectly formatted for BCON
 * ingestion will almost certainly result in undefined behavior
 *
 * The workflow relies on the passed ctx object, which holds a stack of iterator
 * objects, along with metadata (if the emedded layer is an array, and which
 * element it is on if so).  We iterate, generating tokens from the va_list,
 * until we reach an END token.  If any errors occur, we just blow up (the
 * var_args stuff is already incredibly fragile to mistakes, and we have no way
 * of introspecting, so just don't screw it up).
 *
 * There are also a few STACK_* macros in here which manimpulate ctx that are
 * defined up top.
 *
 * The function returns true if all tokens could be successfully matched, false
 * otherwise.
 * */
bool
bcon_extract_ctx_va (bson_t *bson, bcon_extract_ctx_t *ctx, va_list *ap)
{
   bcon_type_t type;
   const char *key;
   bson_iter_t root_iter;
   bson_iter_t current_iter;
   char i_str[16];

   bcon_extract_t u = {0};

   bson_iter_init (&root_iter, bson);

   while (1) {
      if (STACK_IS_ARRAY) {
         bson_uint32_to_string (STACK_I, &key, i_str, sizeof i_str);
         STACK_I++;
      } else {
         type = _bcon_extract_tokenize (ap, &u);

         if (type == BCON_TYPE_END) {
            return true;
         }

         if (type == BCON_TYPE_DOC_END) {
            STACK_POP_DOC (_noop ());
            continue;
         }

         BSON_ASSERT (type == BCON_TYPE_RAW);

         key = u.key;
      }

      type = _bcon_extract_tokenize (ap, &u);
      BSON_ASSERT (type != BCON_TYPE_END);

      if (type == BCON_TYPE_DOC_END) {
         STACK_POP_DOC (_noop ());
      } else if (type == BCON_TYPE_ARRAY_END) {
         STACK_POP_ARRAY (_noop ());
      } else {
         memcpy (&current_iter, STACK_ITER_CHILD, sizeof current_iter);

         if (!bson_iter_find (&current_iter, key)) {
            return false;
         }

         switch ((int) type) {
         case BCON_TYPE_DOC_START:

            if (bson_iter_type (&current_iter) != BSON_TYPE_DOCUMENT) {
               return false;
            }

            STACK_PUSH_DOC (
               bson_iter_recurse (&current_iter, STACK_ITER_CHILD));
            break;
         case BCON_TYPE_ARRAY_START:

            if (bson_iter_type (&current_iter) != BSON_TYPE_ARRAY) {
               return false;
            }

            STACK_PUSH_ARRAY (
               bson_iter_recurse (&current_iter, STACK_ITER_CHILD));
            break;
         default:

            if (!_bcon_extract_single (&current_iter, type, &u)) {
               return false;
            }

            break;
         }
      }
   }
}

void
bcon_extract_ctx_init (bcon_extract_ctx_t *ctx)
{
   ctx->n = 0;
   ctx->stack[0].is_array = false;
}

bool
bcon_extract (bson_t *bson, ...)
{
   va_list ap;
   bcon_extract_ctx_t ctx;
   bool r;

   bcon_extract_ctx_init (&ctx);

   va_start (ap, bson);

   r = bcon_extract_ctx_va (bson, &ctx, &ap);

   va_end (ap);

   return r;
}


void
bcon_append (bson_t *bson, ...)
{
   va_list ap;
   bcon_append_ctx_t ctx;

   bcon_append_ctx_init (&ctx);

   va_start (ap, bson);

   bcon_append_ctx_va (bson, &ctx, &ap);

   va_end (ap);
}


void
bcon_append_ctx (bson_t *bson, bcon_append_ctx_t *ctx, ...)
{
   va_list ap;

   va_start (ap, ctx);

   bcon_append_ctx_va (bson, ctx, &ap);

   va_end (ap);
}


void
bcon_extract_ctx (bson_t *bson, bcon_extract_ctx_t *ctx, ...)
{
   va_list ap;

   va_start (ap, ctx);

   bcon_extract_ctx_va (bson, ctx, &ap);

   va_end (ap);
}

void
bcon_append_ctx_init (bcon_append_ctx_t *ctx)
{
   ctx->n = 0;
   ctx->stack[0].is_array = 0;
}


bson_t *
bcon_new (void *unused, ...)
{
   va_list ap;
   bcon_append_ctx_t ctx;
   bson_t *bson;

   bcon_append_ctx_init (&ctx);

   bson = bson_new ();

   va_start (ap, unused);

   bcon_append_ctx_va (bson, &ctx, &ap);

   va_end (ap);

   return bson;
}
