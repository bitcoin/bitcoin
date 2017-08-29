/*
 * Copyright 2014 MongoDB, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <math.h>

#include "bson.h"
#include "bson-config.h"
#include "bson-json.h"
#include "bson-iso8601-private.h"
#include "b64_pton.h"

#include "jsonsl/jsonsl.h"

#ifdef _WIN32
#include <io.h>
#include <share.h>
#endif

#ifndef _MSC_VER
#include <strings.h>
#endif

#ifdef _MSC_VER
#define SSCANF sscanf_s
#else
#define SSCANF sscanf
#endif

#define STACK_MAX 100
#define BSON_JSON_DEFAULT_BUF_SIZE (1 << 14)
#define AT_LEAST_0(x) ((x) >= 0 ? (x) : 0)


#define READ_STATE_ENUM(ENUM) BSON_JSON_##ENUM,
#define GENERATE_STRING(STRING) #STRING,

#define FOREACH_READ_STATE(RS)          \
   RS (REGULAR)                         \
   RS (DONE)                            \
   RS (ERROR)                           \
   RS (IN_START_MAP)                    \
   RS (IN_BSON_TYPE)                    \
   RS (IN_BSON_TYPE_DATE_NUMBERLONG)    \
   RS (IN_BSON_TYPE_DATE_ENDMAP)        \
   RS (IN_BSON_TYPE_TIMESTAMP_STARTMAP) \
   RS (IN_BSON_TYPE_TIMESTAMP_VALUES)   \
   RS (IN_BSON_TYPE_TIMESTAMP_ENDMAP)   \
   RS (IN_BSON_TYPE_REGEX_STARTMAP)     \
   RS (IN_BSON_TYPE_REGEX_VALUES)       \
   RS (IN_BSON_TYPE_REGEX_ENDMAP)       \
   RS (IN_BSON_TYPE_BINARY_VALUES)      \
   RS (IN_BSON_TYPE_BINARY_ENDMAP)      \
   RS (IN_BSON_TYPE_SCOPE_STARTMAP)     \
   RS (IN_BSON_TYPE_DBPOINTER_STARTMAP) \
   RS (IN_SCOPE)                        \
   RS (IN_DBPOINTER)

typedef enum { FOREACH_READ_STATE (READ_STATE_ENUM) } bson_json_read_state_t;

static const char *read_state_names[] = {FOREACH_READ_STATE (GENERATE_STRING)};

#define BSON_STATE_ENUM(ENUM) BSON_JSON_LF_##ENUM,

#define FOREACH_BSON_STATE(BS)                                       \
   /* legacy {$regex: "...", $options: "..."} */                     \
   BS (REGEX)                                                        \
   BS (OPTIONS)                                                      \
   /* modern $regularExpression: {pattern: "...", options: "..."} */ \
   BS (REGULAR_EXPRESSION_PATTERN)                                   \
   BS (REGULAR_EXPRESSION_OPTIONS)                                   \
   BS (CODE)                                                         \
   BS (SCOPE)                                                        \
   BS (OID)                                                          \
   BS (BINARY)                                                       \
   BS (TYPE)                                                         \
   BS (DATE)                                                         \
   BS (TIMESTAMP_T)                                                  \
   BS (TIMESTAMP_I)                                                  \
   BS (UNDEFINED)                                                    \
   BS (MINKEY)                                                       \
   BS (MAXKEY)                                                       \
   BS (INT32)                                                        \
   BS (INT64)                                                        \
   BS (DOUBLE)                                                       \
   BS (DECIMAL128)                                                   \
   BS (DBPOINTER)                                                    \
   BS (SYMBOL)                                                       \
   BS (DBREF)

typedef enum {
   FOREACH_BSON_STATE (BSON_STATE_ENUM)
} bson_json_read_bson_state_t;

static const char *bson_state_names[] = {FOREACH_BSON_STATE (GENERATE_STRING)};

typedef struct {
   uint8_t *buf;
   size_t n_bytes;
   size_t len;
} bson_json_buf_t;


typedef enum {
   BSON_JSON_FRAME_ARRAY,
   BSON_JSON_FRAME_DOC,
   BSON_JSON_FRAME_SCOPE,
   BSON_JSON_FRAME_DBPOINTER,
} bson_json_frame_type_t;


typedef struct {
   int i;
   bson_json_frame_type_t type;
   bool has_ref;
   bool has_id;
   bson_t bson;
} bson_json_stack_frame_t;


typedef union {
   struct {
      bool has_pattern;
      bool has_options;
      bool is_legacy;
   } regex;
   struct {
      bool has_oid;
      bson_oid_t oid;
   } oid;
   struct {
      bool has_binary;
      bool has_subtype;
      bson_subtype_t type;
      bool is_legacy;
   } binary;
   struct {
      bool has_date;
      int64_t date;
   } date;
   struct {
      bool has_t;
      bool has_i;
      uint32_t t;
      uint32_t i;
   } timestamp;
   struct {
      bool has_undefined;
   } undefined;
   struct {
      bool has_minkey;
   } minkey;
   struct {
      bool has_maxkey;
   } maxkey;
   struct {
      int32_t value;
   } v_int32;
   struct {
      int64_t value;
   } v_int64;
   struct {
      double value;
   } v_double;
   struct {
      bson_decimal128_t value;
   } v_decimal128;
} bson_json_bson_data_t;


/* collect info while parsing a {$code: "...", $scope: {...}} object */
typedef struct {
   bool has_code;
   bool has_scope;
   bool in_scope;
   bson_json_buf_t key_buf;
   bson_json_buf_t code_buf;
} bson_json_code_t;


static void
_bson_json_code_cleanup (bson_json_code_t *code_data)
{
   bson_free (code_data->key_buf.buf);
   bson_free (code_data->code_buf.buf);
}


typedef struct {
   bson_t *bson;
   bson_json_stack_frame_t stack[STACK_MAX];
   int n;
   const char *key;
   bson_json_buf_t key_buf;
   bson_json_buf_t unescaped;
   bson_json_read_state_t read_state;
   bson_json_read_bson_state_t bson_state;
   bson_type_t bson_type;
   bson_json_buf_t bson_type_buf[3];
   bson_json_bson_data_t bson_type_data;
   bson_json_code_t code_data;
   bson_json_buf_t dbpointer_key;
} bson_json_reader_bson_t;


typedef struct {
   void *data;
   bson_json_reader_cb cb;
   bson_json_destroy_cb dcb;
   uint8_t *buf;
   size_t buf_size;
   size_t bytes_read;
   size_t bytes_parsed;
   bool all_whitespace;
} bson_json_reader_producer_t;


struct _bson_json_reader_t {
   bson_json_reader_producer_t producer;
   bson_json_reader_bson_t bson;
   jsonsl_t json;
   ssize_t json_text_pos;
   bool should_reset;
   ssize_t advance;
   bson_json_buf_t tok_accumulator;
   bson_error_t *error;
};


typedef struct {
   int fd;
   bool do_close;
} bson_json_reader_handle_fd_t;


/* forward decl */
static void
_bson_json_save_map_key (bson_json_reader_bson_t *bson,
                         const uint8_t *val,
                         size_t len);


static void
_noop (void)
{
}

#define STACK_ELE(_delta, _name) (bson->stack[(_delta) + bson->n]._name)
#define STACK_BSON(_delta) \
   (((_delta) + bson->n) == 0 ? bson->bson : &STACK_ELE (_delta, bson))
#define STACK_BSON_PARENT STACK_BSON (-1)
#define STACK_BSON_CHILD STACK_BSON (0)
#define STACK_I STACK_ELE (0, i)
#define STACK_FRAME_TYPE STACK_ELE (0, type)
#define STACK_IS_ARRAY (STACK_FRAME_TYPE == BSON_JSON_FRAME_ARRAY)
#define STACK_IS_DOC (STACK_FRAME_TYPE == BSON_JSON_FRAME_DOC)
#define STACK_IS_SCOPE (STACK_FRAME_TYPE == BSON_JSON_FRAME_SCOPE)
#define STACK_IS_DBPOINTER (STACK_FRAME_TYPE == BSON_JSON_FRAME_DBPOINTER)
#define STACK_HAS_REF STACK_ELE (0, has_ref)
#define STACK_HAS_ID STACK_ELE (0, has_id)
#define STACK_PUSH_ARRAY(statement)             \
   do {                                         \
      if (bson->n >= (STACK_MAX - 1)) {         \
         return;                                \
      }                                         \
      bson->n++;                                \
      STACK_I = 0;                              \
      STACK_FRAME_TYPE = BSON_JSON_FRAME_ARRAY; \
      if (bson->n != 0) {                       \
         statement;                             \
      }                                         \
   } while (0)
#define STACK_PUSH_DOC(statement)             \
   do {                                       \
      if (bson->n >= (STACK_MAX - 1)) {       \
         return;                              \
      }                                       \
      bson->n++;                              \
      STACK_FRAME_TYPE = BSON_JSON_FRAME_DOC; \
      STACK_HAS_REF = false;                  \
      STACK_HAS_ID = false;                   \
      if (bson->n != 0) {                     \
         statement;                           \
      }                                       \
   } while (0)
#define STACK_PUSH_SCOPE(statement)             \
   do {                                         \
      if (bson->n >= (STACK_MAX - 1)) {         \
         return;                                \
      }                                         \
      bson->n++;                                \
      STACK_FRAME_TYPE = BSON_JSON_FRAME_SCOPE; \
      bson->code_data.in_scope = true;          \
      if (bson->n != 0) {                       \
         statement;                             \
      }                                         \
   } while (0)
#define STACK_PUSH_DBPOINTER(statement)             \
   do {                                             \
      if (bson->n >= (STACK_MAX - 1)) {             \
         return;                                    \
      }                                             \
      bson->n++;                                    \
      STACK_FRAME_TYPE = BSON_JSON_FRAME_DBPOINTER; \
      if (bson->n != 0) {                           \
         statement;                                 \
      }                                             \
   } while (0)
#define STACK_POP_ARRAY(statement) \
   do {                            \
      if (!STACK_IS_ARRAY) {       \
         return;                   \
      }                            \
      if (bson->n < 0) {           \
         return;                   \
      }                            \
      if (bson->n > 0) {           \
         statement;                \
      }                            \
      bson->n--;                   \
   } while (0)
#define STACK_POP_DOC(statement) \
   do {                          \
      if (STACK_IS_ARRAY) {      \
         return;                 \
      }                          \
      if (bson->n < 0) {         \
         return;                 \
      }                          \
      if (bson->n > 0) {         \
         statement;              \
      }                          \
      bson->n--;                 \
   } while (0)
#define STACK_POP_SCOPE                 \
   do {                                 \
      STACK_POP_DOC (_noop ());         \
      bson->code_data.in_scope = false; \
   } while (0);
#define STACK_POP_DBPOINTER STACK_POP_DOC (_noop ())
#define BASIC_CB_PREAMBLE                         \
   const char *key;                               \
   size_t len;                                    \
   bson_json_reader_bson_t *bson = &reader->bson; \
   _bson_json_read_fixup_key (bson);              \
   key = bson->key;                               \
   len = bson->key_buf.len;
#define BASIC_CB_BAIL_IF_NOT_NORMAL(_type)                                     \
   if (bson->read_state != BSON_JSON_REGULAR) {                                \
      _bson_json_read_set_error (reader,                                       \
                                 "Invalid read of %s in state %s",             \
                                 (_type),                                      \
                                 read_state_names[bson->read_state]);          \
      return;                                                                  \
   } else if (!key) {                                                          \
      _bson_json_read_set_error (reader,                                       \
                                 "Invalid read of %s without key in state %s", \
                                 (_type),                                      \
                                 read_state_names[bson->read_state]);          \
      return;                                                                  \
   }
#define HANDLE_OPTION(_key, _type, _state)                                     \
   (len == strlen (_key) && strncmp ((const char *) val, (_key), len) == 0)    \
   {                                                                           \
      if (bson->bson_type && bson->bson_type != (_type)) {                     \
         _bson_json_read_set_error (reader,                                    \
                                    "Invalid key \"%s\".  Looking for values " \
                                    "for type \"%s\", got \"%s\"",             \
                                    (_key),                                    \
                                    _bson_json_type_name (bson->bson_type),    \
                                    _bson_json_type_name (_type));             \
         return;                                                               \
      }                                                                        \
      bson->bson_type = (_type);                                               \
      bson->bson_state = (_state);                                             \
   }


static void
_bson_json_read_set_error (bson_json_reader_t *reader, const char *fmt, ...)
   BSON_GNUC_PRINTF (2, 3);


static void
_bson_json_read_set_error (bson_json_reader_t *reader, /* IN */
                           const char *fmt,            /* IN */
                           ...)
{
   va_list ap;

   if (reader->error) {
      reader->error->domain = BSON_ERROR_JSON;
      reader->error->code = BSON_JSON_ERROR_READ_INVALID_PARAM;
      va_start (ap, fmt);
      bson_vsnprintf (
         reader->error->message, sizeof reader->error->message, fmt, ap);
      va_end (ap);
      reader->error->message[sizeof reader->error->message - 1] = '\0';
   }

   reader->bson.read_state = BSON_JSON_ERROR;
   jsonsl_stop (reader->json);
}


static void
_bson_json_read_corrupt (bson_json_reader_t *reader, const char *fmt, ...)
   BSON_GNUC_PRINTF (2, 3);


static void
_bson_json_read_corrupt (bson_json_reader_t *reader, /* IN */
                         const char *fmt,            /* IN */
                         ...)
{
   va_list ap;

   if (reader->error) {
      reader->error->domain = BSON_ERROR_JSON;
      reader->error->code = BSON_JSON_ERROR_READ_CORRUPT_JS;
      va_start (ap, fmt);
      bson_vsnprintf (
         reader->error->message, sizeof reader->error->message, fmt, ap);
      va_end (ap);
      reader->error->message[sizeof reader->error->message - 1] = '\0';
   }

   reader->bson.read_state = BSON_JSON_ERROR;
   jsonsl_stop (reader->json);
}


static void
_bson_json_buf_ensure (bson_json_buf_t *buf, /* IN */
                       size_t len)           /* IN */
{
   if (buf->n_bytes < len) {
      bson_free (buf->buf);

      buf->n_bytes = bson_next_power_of_two (len);
      buf->buf = bson_malloc (buf->n_bytes);
   }
}


static void
_bson_json_buf_set (bson_json_buf_t *buf, const void *from, size_t len)
{
   _bson_json_buf_ensure (buf, len + 1);
   memcpy (buf->buf, from, len);
   buf->buf[len] = '\0';
   buf->len = len;
}


static void
_bson_json_buf_append (bson_json_buf_t *buf, const void *from, size_t len)
{
   size_t len_with_null = len + 1;

   if (buf->len == 0) {
      _bson_json_buf_ensure (buf, len_with_null);
   } else if (buf->n_bytes < buf->len + len_with_null) {
      buf->n_bytes = bson_next_power_of_two (buf->len + len_with_null);
      buf->buf = bson_realloc (buf->buf, buf->n_bytes);
   }

   memcpy (buf->buf + buf->len, from, len);
   buf->len += len;
   buf->buf[buf->len] = '\0';
}


static const char *
_bson_json_type_name (bson_type_t type)
{
   switch (type) {
   case BSON_TYPE_EOD:
      return "end of document";
   case BSON_TYPE_DOUBLE:
      return "double";
   case BSON_TYPE_UTF8:
      return "utf-8";
   case BSON_TYPE_DOCUMENT:
      return "document";
   case BSON_TYPE_ARRAY:
      return "array";
   case BSON_TYPE_BINARY:
      return "binary";
   case BSON_TYPE_UNDEFINED:
      return "undefined";
   case BSON_TYPE_OID:
      return "objectid";
   case BSON_TYPE_BOOL:
      return "bool";
   case BSON_TYPE_DATE_TIME:
      return "datetime";
   case BSON_TYPE_NULL:
      return "null";
   case BSON_TYPE_REGEX:
      return "regex";
   case BSON_TYPE_DBPOINTER:
      return "dbpointer";
   case BSON_TYPE_CODE:
      return "code";
   case BSON_TYPE_SYMBOL:
      return "symbol";
   case BSON_TYPE_CODEWSCOPE:
      return "code with scope";
   case BSON_TYPE_INT32:
      return "int32";
   case BSON_TYPE_TIMESTAMP:
      return "timestamp";
   case BSON_TYPE_INT64:
      return "int64";
   case BSON_TYPE_DECIMAL128:
      return "decimal128";
   case BSON_TYPE_MAXKEY:
      return "maxkey";
   case BSON_TYPE_MINKEY:
      return "minkey";
   default:
      return "";
   }
}


static void
_bson_json_read_fixup_key (bson_json_reader_bson_t *bson) /* IN */
{
   bson_json_read_state_t rs = bson->read_state;

   if (bson->n >= 0 && STACK_IS_ARRAY && rs == BSON_JSON_REGULAR) {
      _bson_json_buf_ensure (&bson->key_buf, 12);
      bson->key_buf.len = bson_uint32_to_string (
         STACK_I, &bson->key, (char *) bson->key_buf.buf, 12);
      STACK_I++;
   }
}


static void
_bson_json_read_null (bson_json_reader_t *reader)
{
   BASIC_CB_PREAMBLE;
   BASIC_CB_BAIL_IF_NOT_NORMAL ("null");

   bson_append_null (STACK_BSON_CHILD, key, (int) len);
}


static void
_bson_json_read_boolean (bson_json_reader_t *reader, /* IN */
                         int val)                    /* IN */
{
   BASIC_CB_PREAMBLE;

   if (bson->read_state == BSON_JSON_IN_BSON_TYPE &&
       bson->bson_state == BSON_JSON_LF_UNDEFINED) {
      bson->bson_type_data.undefined.has_undefined = true;
      return;
   }

   BASIC_CB_BAIL_IF_NOT_NORMAL ("boolean");

   bson_append_bool (STACK_BSON_CHILD, key, (int) len, val);
}


/* sign is -1 or 1 */
static void
_bson_json_read_integer (bson_json_reader_t *reader, uint64_t val, int64_t sign)
{
   bson_json_read_state_t rs;
   bson_json_read_bson_state_t bs;

   BASIC_CB_PREAMBLE;

   if (sign == 1 && val > INT64_MAX) {
      _bson_json_read_set_error (
         reader, "Number \"%" PRIu64 "\" is out of range", val);

      return;
   } else if (sign == -1 && val > ((uint64_t) INT64_MAX + 1)) {
      _bson_json_read_set_error (
         reader, "Number \"-%" PRIu64 "\" is out of range", val);

      return;
   }

   rs = bson->read_state;
   bs = bson->bson_state;

   if (rs == BSON_JSON_REGULAR) {
      BASIC_CB_BAIL_IF_NOT_NORMAL ("integer");

      if (val <= INT32_MAX || (sign == -1 && val <= (uint64_t) INT32_MAX + 1)) {
         bson_append_int32 (
            STACK_BSON_CHILD, key, (int) len, (int) (val * sign));
      } else if (sign == -1) {
         bson_append_int64 (STACK_BSON_CHILD, key, (int) len, (int64_t) -val);
      } else {
         bson_append_int64 (STACK_BSON_CHILD, key, (int) len, (int64_t) val);
      }
   } else if (rs == BSON_JSON_IN_BSON_TYPE ||
              rs == BSON_JSON_IN_BSON_TYPE_TIMESTAMP_VALUES) {
      switch (bs) {
      case BSON_JSON_LF_DATE:
         bson->bson_type_data.date.has_date = true;
         bson->bson_type_data.date.date = sign * val;
         break;
      case BSON_JSON_LF_TIMESTAMP_T:
         if (sign == -1) {
            _bson_json_read_set_error (
               reader, "Invalid timestamp value: \"-%" PRIu64 "\"", val);
            return;
         }

         bson->bson_type_data.timestamp.has_t = true;
         bson->bson_type_data.timestamp.t = (uint32_t) val;
         break;
      case BSON_JSON_LF_TIMESTAMP_I:
         if (sign == -1) {
            _bson_json_read_set_error (
               reader, "Invalid timestamp value: \"-%" PRIu64 "\"", val);
            return;
         }

         bson->bson_type_data.timestamp.has_i = true;
         bson->bson_type_data.timestamp.i = (uint32_t) val;
         break;
      case BSON_JSON_LF_MINKEY:
         if (sign == -1) {
            _bson_json_read_set_error (
               reader, "Invalid MinKey value: \"-%" PRIu64 "\"", val);
            return;
         } else if (val != 1) {
            _bson_json_read_set_error (
               reader, "Invalid MinKey value: \"%" PRIu64 "\"", val);
         }

         bson->bson_type_data.minkey.has_minkey = true;
         break;
      case BSON_JSON_LF_MAXKEY:
         if (sign == -1) {
            _bson_json_read_set_error (
               reader, "Invalid MinKey value: \"-%" PRIu64 "\"", val);
            return;
         } else if (val != 1) {
            _bson_json_read_set_error (
               reader, "Invalid MinKey value: \"%" PRIu64 "\"", val);
         }

         bson->bson_type_data.maxkey.has_maxkey = true;
         break;
      case BSON_JSON_LF_INT32:
      case BSON_JSON_LF_INT64:
         _bson_json_read_set_error (
            reader,
            "Invalid state for integer read: %s, "
            "expected number as quoted string like \"123\"",
            bson_state_names[bs]);
         break;
      case BSON_JSON_LF_REGEX:
      case BSON_JSON_LF_OPTIONS:
      case BSON_JSON_LF_REGULAR_EXPRESSION_PATTERN:
      case BSON_JSON_LF_REGULAR_EXPRESSION_OPTIONS:
      case BSON_JSON_LF_CODE:
      case BSON_JSON_LF_SCOPE:
      case BSON_JSON_LF_OID:
      case BSON_JSON_LF_BINARY:
      case BSON_JSON_LF_TYPE:
      case BSON_JSON_LF_UNDEFINED:
      case BSON_JSON_LF_DOUBLE:
      case BSON_JSON_LF_DECIMAL128:
      case BSON_JSON_LF_DBPOINTER:
      case BSON_JSON_LF_SYMBOL:
      case BSON_JSON_LF_DBREF:
      default:
         _bson_json_read_set_error (reader,
                                    "Unexpected integer %s%" PRIu64
                                    " in type \"%s\"",
                                    sign == -1 ? "-" : "",
                                    val,
                                    _bson_json_type_name (bson->bson_type));
      }
   } else {
      _bson_json_read_set_error (reader,
                                 "Unexpected integer %s%" PRIu64
                                 " in state \"%s\"",
                                 sign == -1 ? "-" : "",
                                 val,
                                 read_state_names[rs]);
   }
}


static bool
_bson_json_parse_double (bson_json_reader_t *reader,
                         const char *val,
                         size_t vlen,
                         double *d)
{
   errno = 0;
   *d = strtod (val, NULL);

#ifdef _MSC_VER
   /* Microsoft's strtod parses "NaN", "Infinity", "-Infinity" as 0 */
   if (*d == 0.0) {
      if (!_strnicmp (val, "nan", vlen)) {
#ifdef NAN
         *d = NAN;
#else
         /* Visual Studio 2010 doesn't define NAN or INFINITY
          * https://msdn.microsoft.com/en-us/library/w22adx1s(v=vs.100).aspx */
         unsigned long nan[2] = {0xffffffff, 0x7fffffff};
         *d = *(double *) nan;
#endif
         return true;
      } else if (!_strnicmp (val, "infinity", vlen)) {
#ifdef INFINITY
         *d = INFINITY;
#else
         unsigned long inf[2] = {0x00000000, 0x7ff00000};
         *d = *(double *) inf;
#endif
         return true;
      } else if (!_strnicmp (val, "-infinity", vlen)) {
#ifdef INFINITY
         *d = -INFINITY;
#else
         unsigned long inf[2] = {0x00000000, 0xfff00000};
         *d = *(double *) inf;
#endif
         return true;
      }
   }

   if ((*d == HUGE_VAL || *d == -HUGE_VAL) && errno == ERANGE) {
      _bson_json_read_set_error (
         reader, "Number \"%.*s\" is out of range", (int) vlen, val);

      return false;
   }
#else
   /* not MSVC -  set err on overflow, but avoid err for infinity */
   if ((*d == HUGE_VAL || *d == -HUGE_VAL) && errno == ERANGE &&
       strncasecmp (val, "infinity", vlen) &&
       strncasecmp (val, "-infinity", vlen)) {
      _bson_json_read_set_error (
         reader, "Number \"%.*s\" is out of range", (int) vlen, val);

      return false;
   }

#endif /* _MSC_VER */
   return true;
}


static void
_bson_json_read_double (bson_json_reader_t *reader, /* IN */
                        double val)                 /* IN */
{
   BASIC_CB_PREAMBLE;
   BASIC_CB_BAIL_IF_NOT_NORMAL ("double");

   bson_append_double (STACK_BSON_CHILD, key, (int) len, val);
}


static bool
_bson_json_read_int64_or_set_error (bson_json_reader_t *reader, /* IN */
                                    const unsigned char *val,   /* IN */
                                    size_t vlen,                /* IN */
                                    int64_t *v64)               /* OUT */
{
   bson_json_reader_bson_t *bson = &reader->bson;
   char *endptr = NULL;

   _bson_json_read_fixup_key (bson);
   errno = 0;
   *v64 = bson_ascii_strtoll ((const char *) val, &endptr, 10);

   if (((*v64 == INT64_MIN) || (*v64 == INT64_MAX)) && (errno == ERANGE)) {
      _bson_json_read_set_error (reader, "Number \"%s\" is out of range", val);
      return false;
   }

   if (endptr != ((const char *) val + vlen)) {
      _bson_json_read_set_error (reader, "Number \"%s\" is invalid", val);
      return false;
   }

   return true;
}


/* parse a value for "base64", "subType" or legacy "$binary" or "$type" */
static void
_bson_json_parse_binary_elem (bson_json_reader_t *reader,
                              const char *val_w_null,
                              size_t vlen)
{
   bson_json_read_bson_state_t bs;
   bson_json_bson_data_t *data;
   int binary_len;

   BASIC_CB_PREAMBLE;

   bs = bson->bson_state;
   data = &bson->bson_type_data;

   if (bs == BSON_JSON_LF_BINARY) {
      data->binary.has_binary = true;
      binary_len = b64_pton (val_w_null, NULL, 0);
      if (binary_len < 0) {
         _bson_json_read_set_error (
            reader,
            "Invalid input string \"%s\", looking for base64-encoded binary",
            val_w_null);
      }

      _bson_json_buf_ensure (&bson->bson_type_buf[0], (size_t) binary_len + 1);
      b64_pton (
         val_w_null, bson->bson_type_buf[0].buf, (size_t) binary_len + 1);

      bson->bson_type_buf[0].len = (size_t) binary_len;
   } else if (bs == BSON_JSON_LF_TYPE) {
      data->binary.has_subtype = true;

      if (SSCANF (val_w_null, "%02x", &data->binary.type) != 1) {
         if (!data->binary.is_legacy || data->binary.has_binary) {
            /* misformatted subtype, like {$binary: {base64: "", subType: "x"}},
             * or legacy {$binary: "", $type: "x"} */
            _bson_json_read_set_error (
               reader,
               "Invalid input string \"%s\", looking for binary subtype",
               val_w_null);
         } else {
            /* actually a query operator: {x: {$type: "array"}}*/
            bson->read_state = BSON_JSON_REGULAR;
            STACK_PUSH_DOC (bson_append_document_begin (
               STACK_BSON_PARENT, key, (int) len, STACK_BSON_CHILD));

            bson_append_utf8 (STACK_BSON_CHILD,
                              "$type",
                              5,
                              (const char *) val_w_null,
                              (int) vlen);
         }
      }
   }
}


static void
_bson_json_read_string (bson_json_reader_t *reader, /* IN */
                        const unsigned char *val,   /* IN */
                        size_t vlen)                /* IN */
{
   bson_json_read_state_t rs;
   bson_json_read_bson_state_t bs;

   BASIC_CB_PREAMBLE;

   rs = bson->read_state;
   bs = bson->bson_state;

   if (!bson_utf8_validate ((const char *) val, vlen, true /*allow null*/)) {
      _bson_json_read_corrupt (reader, "invalid bytes in UTF8 string");
      return;
   }

   if (rs == BSON_JSON_REGULAR) {
      BASIC_CB_BAIL_IF_NOT_NORMAL ("string");
      bson_append_utf8 (
         STACK_BSON_CHILD, key, (int) len, (const char *) val, (int) vlen);
   } else if (rs == BSON_JSON_IN_BSON_TYPE_SCOPE_STARTMAP ||
              rs == BSON_JSON_IN_BSON_TYPE_DBPOINTER_STARTMAP) {
      _bson_json_read_set_error (reader,
                                 "Invalid read of \"%s\" in state \"%s\"",
                                 val,
                                 read_state_names[rs]);
   } else if (rs == BSON_JSON_IN_BSON_TYPE_BINARY_VALUES) {
      const char *val_w_null;
      _bson_json_buf_set (&bson->bson_type_buf[2], val, vlen);
      val_w_null = (const char *) bson->bson_type_buf[2].buf;

      _bson_json_parse_binary_elem (reader, val_w_null, vlen);
   } else if (rs == BSON_JSON_IN_BSON_TYPE ||
              rs == BSON_JSON_IN_BSON_TYPE_TIMESTAMP_VALUES ||
              rs == BSON_JSON_IN_BSON_TYPE_REGEX_VALUES ||
              rs == BSON_JSON_IN_BSON_TYPE_DATE_NUMBERLONG) {
      const char *val_w_null;
      _bson_json_buf_set (&bson->bson_type_buf[2], val, vlen);
      val_w_null = (const char *) bson->bson_type_buf[2].buf;

      switch (bs) {
      case BSON_JSON_LF_REGEX:
         bson->bson_type_data.regex.is_legacy = true;
      /* FALL THROUGH */
      case BSON_JSON_LF_REGULAR_EXPRESSION_PATTERN:
         bson->bson_type_data.regex.has_pattern = true;
         _bson_json_buf_set (&bson->bson_type_buf[0], val, vlen);
         break;
      case BSON_JSON_LF_OPTIONS:
         bson->bson_type_data.regex.is_legacy = true;
      /* FALL THROUGH */
      case BSON_JSON_LF_REGULAR_EXPRESSION_OPTIONS:
         bson->bson_type_data.regex.has_options = true;
         _bson_json_buf_set (&bson->bson_type_buf[1], val, vlen);
         break;
      case BSON_JSON_LF_OID:

         if (vlen != 24) {
            goto BAD_PARSE;
         }

         bson->bson_type_data.oid.has_oid = true;
         bson_oid_init_from_string (&bson->bson_type_data.oid.oid, val_w_null);
         break;
      case BSON_JSON_LF_BINARY:
      case BSON_JSON_LF_TYPE:
         bson->bson_type_data.binary.is_legacy = true;
         _bson_json_parse_binary_elem (reader, val_w_null, vlen);
         break;
      case BSON_JSON_LF_INT32: {
         int64_t v64;
         if (!_bson_json_read_int64_or_set_error (reader, val, vlen, &v64)) {
            /* the error is set, return and let the reader exit */
            return;
         }

         if (v64 < INT32_MIN || v64 > INT32_MAX) {
            goto BAD_PARSE;
         }

         if (bson->read_state == BSON_JSON_IN_BSON_TYPE) {
            bson->bson_type_data.v_int32.value = (int32_t) v64;
         } else {
            goto BAD_PARSE;
         }
      } break;
      case BSON_JSON_LF_INT64: {
         int64_t v64;
         if (!_bson_json_read_int64_or_set_error (reader, val, vlen, &v64)) {
            /* the error is set, return and let the reader exit */
            return;
         }

         if (bson->read_state == BSON_JSON_IN_BSON_TYPE) {
            bson->bson_type_data.v_int64.value = v64;
         } else if (bson->read_state ==
                    BSON_JSON_IN_BSON_TYPE_DATE_NUMBERLONG) {
            bson->bson_type_data.date.has_date = true;
            bson->bson_type_data.date.date = v64;
         } else {
            goto BAD_PARSE;
         }
      } break;
      case BSON_JSON_LF_DOUBLE: {
         _bson_json_parse_double (reader,
                                  (const char *) val,
                                  vlen,
                                  &bson->bson_type_data.v_double.value);
      } break;
      case BSON_JSON_LF_DATE: {
         int64_t v64;

         if (!_bson_iso8601_date_parse (
                (char *) val, (int) vlen, &v64, reader->error)) {
            jsonsl_stop (reader->json);
         } else {
            bson->bson_type_data.date.has_date = true;
            bson->bson_type_data.date.date = v64;
         }
      } break;
      case BSON_JSON_LF_DECIMAL128: {
         bson_decimal128_t decimal128;
         bson_decimal128_from_string (val_w_null, &decimal128);

         if (bson->read_state == BSON_JSON_IN_BSON_TYPE) {
            bson->bson_type_data.v_decimal128.value = decimal128;
         } else {
            goto BAD_PARSE;
         }
      } break;
      case BSON_JSON_LF_CODE:
         _bson_json_buf_set (&bson->code_data.code_buf, val, vlen);
         break;
      case BSON_JSON_LF_SYMBOL:
         bson_append_symbol (
            STACK_BSON_CHILD, key, (int) len, (const char *) val, (int) vlen);
         break;
      case BSON_JSON_LF_DBREF:
         /* the "$ref" of a {$ref: "...", $id: ... }, append normally */
         bson_append_utf8 (
            STACK_BSON_CHILD, key, (int) len, (const char *) val, (int) vlen);
         bson->read_state = BSON_JSON_REGULAR;
         break;
      case BSON_JSON_LF_SCOPE:
      case BSON_JSON_LF_TIMESTAMP_T:
      case BSON_JSON_LF_TIMESTAMP_I:
      case BSON_JSON_LF_UNDEFINED:
      case BSON_JSON_LF_MINKEY:
      case BSON_JSON_LF_MAXKEY:
      case BSON_JSON_LF_DBPOINTER:
      default:
         goto BAD_PARSE;
      }

      return;
   BAD_PARSE:
      _bson_json_read_set_error (reader,
                                 "Invalid input string \"%s\", looking for %s",
                                 val_w_null,
                                 bson_state_names[bs]);
   } else {
      _bson_json_read_set_error (
         reader, "Invalid state to look for string: %s", read_state_names[rs]);
   }
}


static void
_bson_json_read_start_map (bson_json_reader_t *reader) /* IN */
{
   BASIC_CB_PREAMBLE;

   if (bson->read_state == BSON_JSON_IN_BSON_TYPE) {
      if (bson->bson_state == BSON_JSON_LF_DATE) {
         bson->read_state = BSON_JSON_IN_BSON_TYPE_DATE_NUMBERLONG;
      } else if (bson->bson_state == BSON_JSON_LF_BINARY) {
         bson->read_state = BSON_JSON_IN_BSON_TYPE_BINARY_VALUES;
      } else if (bson->bson_state == BSON_JSON_LF_TYPE) {
         /* special case, we started parsing {$type: {$numberInt: "2"}} and we
          * expected a legacy Binary format. now we see the second "{", so
          * backtrack and parse $type query operator. */
         bson->read_state = BSON_JSON_IN_START_MAP;
         STACK_PUSH_DOC (bson_append_document_begin (
            STACK_BSON_PARENT, key, len, STACK_BSON_CHILD));
         _bson_json_save_map_key (bson, (const uint8_t *) "$type", 5);
      }
   } else if (bson->read_state == BSON_JSON_IN_BSON_TYPE_TIMESTAMP_STARTMAP) {
      bson->read_state = BSON_JSON_IN_BSON_TYPE_TIMESTAMP_VALUES;
   } else if (bson->read_state == BSON_JSON_IN_BSON_TYPE_SCOPE_STARTMAP) {
      bson->read_state = BSON_JSON_IN_SCOPE;
   } else if (bson->read_state == BSON_JSON_IN_BSON_TYPE_DBPOINTER_STARTMAP) {
      bson->read_state = BSON_JSON_IN_DBPOINTER;
   } else if (bson->read_state == BSON_JSON_IN_BSON_TYPE_REGEX_STARTMAP) {
      bson->read_state = BSON_JSON_IN_BSON_TYPE_REGEX_VALUES;
   } else {
      bson->read_state = BSON_JSON_IN_START_MAP;
   }

   /* silence some warnings */
   (void) len;
   (void) key;
}


static bool
_is_known_key (const char *key, size_t len)
{
   bool ret;

#define IS_KEY(k) (len == strlen (k) && (0 == memcmp (k, key, len)))

   ret = (IS_KEY ("$regularExpression") || IS_KEY ("$regex") ||
          IS_KEY ("$options") || IS_KEY ("$code") || IS_KEY ("$scope") ||
          IS_KEY ("$oid") || IS_KEY ("$binary") || IS_KEY ("$type") ||
          IS_KEY ("$date") || IS_KEY ("$undefined") || IS_KEY ("$maxKey") ||
          IS_KEY ("$minKey") || IS_KEY ("$timestamp") ||
          IS_KEY ("$numberInt") || IS_KEY ("$numberLong") ||
          IS_KEY ("$numberDouble") || IS_KEY ("$numberDecimal") ||
          IS_KEY ("$numberInt") || IS_KEY ("$numberLong") ||
          IS_KEY ("$numberDouble") || IS_KEY ("$numberDecimal") ||
          IS_KEY ("$dbPointer") || IS_KEY ("$symbol"));

#undef IS_KEY

   return ret;
}

static void
_bson_json_save_map_key (bson_json_reader_bson_t *bson,
                         const uint8_t *val,
                         size_t len)
{
   _bson_json_buf_set (&bson->key_buf, val, len);
   bson->key = (const char *) bson->key_buf.buf;
}


static void
_bson_json_read_code_or_scope_key (bson_json_reader_bson_t *bson,
                                   bool is_scope,
                                   const uint8_t *val,
                                   size_t len)
{
   bson_json_code_t *code = &bson->code_data;

   if (code->in_scope) {
      /* we're reading something weirdly nested, e.g. we just read "$code" in
       * "$scope: {x: {$code: {}}}". just create the subdoc within the scope. */
      bson->read_state = BSON_JSON_REGULAR;
      STACK_PUSH_DOC (bson_append_document_begin (STACK_BSON_PARENT,
                                                  bson->key,
                                                  (int) bson->key_buf.len,
                                                  STACK_BSON_CHILD));
      _bson_json_save_map_key (bson, val, len);
   } else {
      if (!bson->code_data.key_buf.len) {
         /* save the key, e.g. {"key": {"$code": "return x", "$scope":{"x":1}}},
          * in case it is overwritten while parsing scope sub-object */
         _bson_json_buf_set (
            &bson->code_data.key_buf, bson->key_buf.buf, bson->key_buf.len);
      }

      if (is_scope) {
         bson->bson_type = BSON_TYPE_CODEWSCOPE;
         bson->read_state = BSON_JSON_IN_BSON_TYPE_SCOPE_STARTMAP;
         bson->bson_state = BSON_JSON_LF_SCOPE;
         bson->code_data.has_scope = true;
      } else {
         bson->bson_type = BSON_TYPE_CODE;
         bson->bson_state = BSON_JSON_LF_CODE;
         bson->code_data.has_code = true;
      }
   }
}


static void
_bson_json_bad_key_in_type (bson_json_reader_t *reader, /* IN */
                            const uint8_t *val)         /* IN */
{
   bson_json_reader_bson_t *bson = &reader->bson;

   _bson_json_read_set_error (
      reader,
      "Invalid key \"%s\".  Looking for values for type \"%s\"",
      val,
      _bson_json_type_name (bson->bson_type));
}


static void
_bson_json_read_map_key (bson_json_reader_t *reader, /* IN */
                         const uint8_t *val,         /* IN */
                         size_t len)                 /* IN */
{
   bson_json_reader_bson_t *bson = &reader->bson;

   if (!bson_utf8_validate ((const char *) val, len, true /* allow null */)) {
      _bson_json_read_corrupt (reader, "invalid bytes in UTF8 string");
      return;
   }

   if (bson->read_state == BSON_JSON_IN_START_MAP) {
      if (len > 0 && val[0] == '$' && _is_known_key ((const char *) val, len) &&
          bson->n >= 0 /* key is in subdocument */) {
         bson->read_state = BSON_JSON_IN_BSON_TYPE;
         bson->bson_type = (bson_type_t) 0;
         memset (&bson->bson_type_data, 0, sizeof bson->bson_type_data);
      } else {
         bson->read_state = BSON_JSON_REGULAR;
         STACK_PUSH_DOC (bson_append_document_begin (STACK_BSON_PARENT,
                                                     bson->key,
                                                     (int) bson->key_buf.len,
                                                     STACK_BSON_CHILD));
      }
   } else if (bson->read_state == BSON_JSON_IN_SCOPE) {
      /* we've read "key" in {$code: "", $scope: {key: ""}}*/
      bson->read_state = BSON_JSON_REGULAR;
      STACK_PUSH_SCOPE (bson_init (STACK_BSON_CHILD));
      _bson_json_save_map_key (bson, val, len);
   } else if (bson->read_state == BSON_JSON_IN_DBPOINTER) {
      /* we've read "$ref" or "$id" in {$dbPointer: {$ref: ..., $id: ...}} */
      bson->read_state = BSON_JSON_REGULAR;
      STACK_PUSH_DBPOINTER (bson_init (STACK_BSON_CHILD));
      _bson_json_save_map_key (bson, val, len);
   }

   if (bson->read_state == BSON_JSON_IN_BSON_TYPE) {
      if
         HANDLE_OPTION ("$regex", BSON_TYPE_REGEX, BSON_JSON_LF_REGEX)
      else if
         HANDLE_OPTION ("$options", BSON_TYPE_REGEX, BSON_JSON_LF_OPTIONS)
      else if
         HANDLE_OPTION ("$oid", BSON_TYPE_OID, BSON_JSON_LF_OID)
      else if
         HANDLE_OPTION ("$binary", BSON_TYPE_BINARY, BSON_JSON_LF_BINARY)
      else if
         HANDLE_OPTION ("$type", BSON_TYPE_BINARY, BSON_JSON_LF_TYPE)
      else if
         HANDLE_OPTION ("$date", BSON_TYPE_DATE_TIME, BSON_JSON_LF_DATE)
      else if
         HANDLE_OPTION (
            "$undefined", BSON_TYPE_UNDEFINED, BSON_JSON_LF_UNDEFINED)
      else if
         HANDLE_OPTION ("$minKey", BSON_TYPE_MINKEY, BSON_JSON_LF_MINKEY)
      else if
         HANDLE_OPTION ("$maxKey", BSON_TYPE_MAXKEY, BSON_JSON_LF_MAXKEY)
      else if
         HANDLE_OPTION ("$numberInt", BSON_TYPE_INT32, BSON_JSON_LF_INT32)
      else if
         HANDLE_OPTION ("$numberLong", BSON_TYPE_INT64, BSON_JSON_LF_INT64)
      else if
         HANDLE_OPTION ("$numberDouble", BSON_TYPE_DOUBLE, BSON_JSON_LF_DOUBLE)
      else if
         HANDLE_OPTION ("$symbol", BSON_TYPE_SYMBOL, BSON_JSON_LF_SYMBOL)
      else if
         HANDLE_OPTION (
            "$numberDecimal", BSON_TYPE_DECIMAL128, BSON_JSON_LF_DECIMAL128)
      else if (!strcmp ("$timestamp", (const char *) val)) {
         bson->bson_type = BSON_TYPE_TIMESTAMP;
         bson->read_state = BSON_JSON_IN_BSON_TYPE_TIMESTAMP_STARTMAP;
      } else if (!strcmp ("$regularExpression", (const char *) val)) {
         bson->bson_type = BSON_TYPE_REGEX;
         bson->read_state = BSON_JSON_IN_BSON_TYPE_REGEX_STARTMAP;
      } else if (!strcmp ("$dbPointer", (const char *) val)) {
         /* start parsing "key": {"$dbPointer": {...}}, save "key" for later */
         _bson_json_buf_set (
            &bson->dbpointer_key, bson->key_buf.buf, bson->key_buf.len);

         bson->bson_type = BSON_TYPE_DBPOINTER;
         bson->read_state = BSON_JSON_IN_BSON_TYPE_DBPOINTER_STARTMAP;
      } else if (!strcmp ("$code", (const char *) val)) {
         _bson_json_read_code_or_scope_key (
            bson, false /* is_scope */, val, len);
      } else if (!strcmp ("$scope", (const char *) val)) {
         _bson_json_read_code_or_scope_key (
            bson, true /* is_scope */, val, len);
      } else {
         _bson_json_bad_key_in_type (reader, val);
      }
   } else if (bson->read_state == BSON_JSON_IN_BSON_TYPE_DATE_NUMBERLONG) {
      if
         HANDLE_OPTION ("$numberLong", BSON_TYPE_DATE_TIME, BSON_JSON_LF_INT64)
      else {
         _bson_json_bad_key_in_type (reader, val);
      }
   } else if (bson->read_state == BSON_JSON_IN_BSON_TYPE_TIMESTAMP_VALUES) {
      if
         HANDLE_OPTION ("t", BSON_TYPE_TIMESTAMP, BSON_JSON_LF_TIMESTAMP_T)
      else if
         HANDLE_OPTION ("i", BSON_TYPE_TIMESTAMP, BSON_JSON_LF_TIMESTAMP_I)
      else {
         _bson_json_bad_key_in_type (reader, val);
      }
   } else if (bson->read_state == BSON_JSON_IN_BSON_TYPE_REGEX_VALUES) {
      if
         HANDLE_OPTION (
            "pattern", BSON_TYPE_REGEX, BSON_JSON_LF_REGULAR_EXPRESSION_PATTERN)
      else if
         HANDLE_OPTION (
            "options", BSON_TYPE_REGEX, BSON_JSON_LF_REGULAR_EXPRESSION_OPTIONS)
      else {
         _bson_json_bad_key_in_type (reader, val);
      }
   } else if (bson->read_state == BSON_JSON_IN_BSON_TYPE_BINARY_VALUES) {
      if
         HANDLE_OPTION ("base64", BSON_TYPE_BINARY, BSON_JSON_LF_BINARY)
      else if
         HANDLE_OPTION ("subType", BSON_TYPE_BINARY, BSON_JSON_LF_TYPE)
      else {
         _bson_json_bad_key_in_type (reader, val);
      }
   } else {
      _bson_json_save_map_key (bson, val, len);

      /* in x: {$ref: "collection", $id: {$oid: "..."}, $db: "..." } */
      if (bson->n > 0) {
         if (!strcmp ("$ref", (const char *) val)) {
            STACK_HAS_REF = true;
            bson->read_state = BSON_JSON_IN_BSON_TYPE;
            bson->bson_state = BSON_JSON_LF_DBREF;
         } else if (!strcmp ("$id", (const char *) val)) {
            STACK_HAS_ID = true;
         } else if (!strcmp ("$db", (const char *) val)) {
            bson->read_state = BSON_JSON_IN_BSON_TYPE;
            bson->bson_state = BSON_JSON_LF_DBREF;
         }
      }
   }
}


static void
_bson_json_read_append_binary (bson_json_reader_t *reader,    /* IN */
                               bson_json_reader_bson_t *bson) /* IN */
{
   bson_json_bson_data_t *data = &bson->bson_type_data;

   if (data->binary.is_legacy) {
      if (!data->binary.has_binary) {
         _bson_json_read_set_error (
            reader,
            "Missing \"$binary\" after \"$type\" reading type \"binary\"");
         return;
      } else if (!data->binary.has_subtype) {
         _bson_json_read_set_error (
            reader,
            "Missing \"$type\" after \"$binary\" reading type \"binary\"");
         return;
      }
   } else {
      if (!data->binary.has_binary) {
         _bson_json_read_set_error (
            reader,
            "Missing \"base64\" after \"subType\" reading type \"binary\"");
         return;
      } else if (!data->binary.has_subtype) {
         _bson_json_read_set_error (
            reader,
            "Missing \"subType\" after \"base64\" reading type \"binary\"");
         return;
      }
   }

   if (!bson_append_binary (STACK_BSON_CHILD,
                            bson->key,
                            (int) bson->key_buf.len,
                            data->binary.type,
                            bson->bson_type_buf[0].buf,
                            (uint32_t) bson->bson_type_buf[0].len)) {
      _bson_json_read_set_error (reader, "Error storing binary data");
   }
}


static void
_bson_json_read_append_regex (bson_json_reader_t *reader,    /* IN */
                              bson_json_reader_bson_t *bson) /* IN */
{
   bson_json_bson_data_t *data = &bson->bson_type_data;
   if (data->regex.is_legacy) {
      if (!data->regex.has_pattern) {
         _bson_json_read_set_error (reader,
                                    "Missing \"$regex\" after \"$options\"");
         return;
      }
      if (!data->regex.has_options) {
         _bson_json_read_set_error (reader,
                                    "Missing \"$options\" after \"$regex\"");
         return;
      }
   } else if (!data->regex.has_pattern) {
      _bson_json_read_set_error (
         reader, "Missing \"pattern\" after \"options\" in regular expression");
      return;
   } else if (!data->regex.has_options) {
      _bson_json_read_set_error (
         reader, "Missing \"options\" after \"pattern\" in regular expression");
      return;
   }

   if (!bson_append_regex (STACK_BSON_CHILD,
                           bson->key,
                           (int) bson->key_buf.len,
                           (char *) bson->bson_type_buf[0].buf,
                           (char *) bson->bson_type_buf[1].buf)) {
      _bson_json_read_set_error (reader, "Error storing regex");
   }
}


static void
_bson_json_read_append_code (bson_json_reader_t *reader,    /* IN */
                             bson_json_reader_bson_t *bson) /* IN */
{
   bson_json_code_t *code_data;
   char *code = NULL;
   bson_t *scope = NULL;
   bool r;

   code_data = &bson->code_data;

   BSON_ASSERT (!code_data->in_scope);

   if (!code_data->has_code) {
      _bson_json_read_set_error (reader, "Missing $code after $scope");
      return;
   }

   code = (char *) code_data->code_buf.buf;

   if (code_data->has_scope) {
      scope = STACK_BSON (1);
   }

   /* creates BSON "code" elem, or "code with scope" if scope is not NULL */
   r = bson_append_code_with_scope (STACK_BSON_CHILD,
                                    (const char *) code_data->key_buf.buf,
                                    (int) code_data->key_buf.len,
                                    code,
                                    scope);

   if (!r) {
      _bson_json_read_set_error (reader, "Error storing Javascript code");
   }

   if (scope) {
      bson_destroy (scope);
   }

   /* keep the buffer but truncate it */
   code_data->key_buf.len = 0;
   code_data->has_code = code_data->has_scope = false;
}


static void
_bson_json_read_append_dbpointer (bson_json_reader_t *reader,    /* IN */
                                  bson_json_reader_bson_t *bson) /* IN */
{
   bson_t *db_pointer;
   bson_iter_t iter;
   const char *ns = NULL;
   const bson_oid_t *oid = NULL;
   bool r;

   BSON_ASSERT (reader->bson.dbpointer_key.buf);

   db_pointer = STACK_BSON (1);
   if (!bson_iter_init (&iter, db_pointer)) {
      _bson_json_read_set_error (reader, "Error storing DBPointer");
      return;
   }

   while (bson_iter_next (&iter)) {
      if (!strcmp (bson_iter_key (&iter), "$id")) {
         if (!BSON_ITER_HOLDS_OID (&iter)) {
            _bson_json_read_set_error (
               reader, "$dbPointer.$id must be like {\"$oid\": ...\"}");
            return;
         }

         oid = bson_iter_oid (&iter);
      } else if (!strcmp (bson_iter_key (&iter), "$ref")) {
         if (!BSON_ITER_HOLDS_UTF8 (&iter)) {
            _bson_json_read_set_error (
               reader,
               "$dbPointer.$ref must be a string like \"db.collection\"");
            return;
         }

         ns = bson_iter_utf8 (&iter, NULL);
      } else {
         _bson_json_read_set_error (reader,
                                    "$dbPointer contains invalid key: \"%s\"",
                                    bson_iter_key (&iter));
         return;
      }
   }

   if (!oid || !ns) {
      _bson_json_read_set_error (reader,
                                 "$dbPointer requires both $id and $ref");
      return;
   }

   r = bson_append_dbpointer (STACK_BSON_CHILD,
                              (char *) reader->bson.dbpointer_key.buf,
                              (int) reader->bson.dbpointer_key.len,
                              ns,
                              oid);

   if (!r) {
      _bson_json_read_set_error (reader, "Error storing DBPointer");
   }
}


static void
_bson_json_read_append_oid (bson_json_reader_t *reader,    /* IN */
                            bson_json_reader_bson_t *bson) /* IN */
{
   if (!bson_append_oid (STACK_BSON_CHILD,
                         bson->key,
                         (int) bson->key_buf.len,
                         &bson->bson_type_data.oid.oid)) {
      _bson_json_read_set_error (reader, "Error storing ObjectId");
   }
}


static void
_bson_json_read_append_date_time (bson_json_reader_t *reader,    /* IN */
                                  bson_json_reader_bson_t *bson) /* IN */
{
   if (!bson_append_date_time (STACK_BSON_CHILD,
                               bson->key,
                               (int) bson->key_buf.len,
                               bson->bson_type_data.date.date)) {
      _bson_json_read_set_error (reader, "Error storing datetime");
   }
}


static void
_bson_json_read_append_timestamp (bson_json_reader_t *reader,    /* IN */
                                  bson_json_reader_bson_t *bson) /* IN */
{
   if (!bson->bson_type_data.timestamp.has_t) {
      _bson_json_read_set_error (
         reader, "Missing t after $timestamp in BSON_TYPE_TIMESTAMP");
      return;
   } else if (!bson->bson_type_data.timestamp.has_i) {
      _bson_json_read_set_error (
         reader, "Missing i after $timestamp in BSON_TYPE_TIMESTAMP");
      return;
   }

   bson_append_timestamp (STACK_BSON_CHILD,
                          bson->key,
                          (int) bson->key_buf.len,
                          bson->bson_type_data.timestamp.t,
                          bson->bson_type_data.timestamp.i);
}


static void
_bad_extended_json (bson_json_reader_t *reader)
{
   _bson_json_read_corrupt (reader, "Invalid MongoDB extended JSON");
}


static void
_bson_json_read_end_map (bson_json_reader_t *reader) /* IN */
{
   bson_json_reader_bson_t *bson = &reader->bson;

   if (bson->read_state == BSON_JSON_IN_START_MAP) {
      bson->read_state = BSON_JSON_REGULAR;
      STACK_PUSH_DOC (bson_append_document_begin (STACK_BSON_PARENT,
                                                  bson->key,
                                                  (int) bson->key_buf.len,
                                                  STACK_BSON_CHILD));
   } else if (bson->read_state == BSON_JSON_IN_BSON_TYPE_SCOPE_STARTMAP) {
      bson->read_state = BSON_JSON_REGULAR;
      STACK_PUSH_SCOPE (bson_init (STACK_BSON_CHILD));
   } else if (bson->read_state == BSON_JSON_IN_BSON_TYPE_DBPOINTER_STARTMAP) {
      /* we've read last "}" in "{$dbPointer: {$id: ..., $ref: ...}}" */
      _bson_json_read_append_dbpointer (reader, bson);
      bson->read_state = BSON_JSON_REGULAR;
      return;
   }

   if (bson->read_state == BSON_JSON_IN_BSON_TYPE) {
      if (!bson->key) {
         /* invalid, like {$numberLong: "1"} at the document top level */
         _bad_extended_json (reader);
         return;
      }

      bson->read_state = BSON_JSON_REGULAR;
      switch (bson->bson_type) {
      case BSON_TYPE_REGEX:
         _bson_json_read_append_regex (reader, bson);
         break;
      case BSON_TYPE_CODE:
      case BSON_TYPE_CODEWSCOPE:
         /* we've read the closing "}" in "{$code: ..., $scope: ...}" */
         _bson_json_read_append_code (reader, bson);
         break;
      case BSON_TYPE_OID:
         _bson_json_read_append_oid (reader, bson);
         break;
      case BSON_TYPE_BINARY:
         _bson_json_read_append_binary (reader, bson);
         break;
      case BSON_TYPE_DATE_TIME:
         _bson_json_read_append_date_time (reader, bson);
         break;
      case BSON_TYPE_UNDEFINED:
         bson_append_undefined (
            STACK_BSON_CHILD, bson->key, (int) bson->key_buf.len);
         break;
      case BSON_TYPE_MINKEY:
         bson_append_minkey (
            STACK_BSON_CHILD, bson->key, (int) bson->key_buf.len);
         break;
      case BSON_TYPE_MAXKEY:
         bson_append_maxkey (
            STACK_BSON_CHILD, bson->key, (int) bson->key_buf.len);
         break;
      case BSON_TYPE_INT32:
         bson_append_int32 (STACK_BSON_CHILD,
                            bson->key,
                            (int) bson->key_buf.len,
                            bson->bson_type_data.v_int32.value);
         break;
      case BSON_TYPE_INT64:
         bson_append_int64 (STACK_BSON_CHILD,
                            bson->key,
                            (int) bson->key_buf.len,
                            bson->bson_type_data.v_int64.value);
         break;
      case BSON_TYPE_DOUBLE:
         bson_append_double (STACK_BSON_CHILD,
                             bson->key,
                             (int) bson->key_buf.len,
                             bson->bson_type_data.v_double.value);
         break;
      case BSON_TYPE_DECIMAL128:
         bson_append_decimal128 (STACK_BSON_CHILD,
                                 bson->key,
                                 (int) bson->key_buf.len,
                                 &bson->bson_type_data.v_decimal128.value);
         break;
      case BSON_TYPE_DBPOINTER:
         /* shouldn't set type to DBPointer unless inside $dbPointer: {...} */
         _bson_json_read_set_error (
            reader,
            "Internal error: shouldn't be in state BSON_TYPE_DBPOINTER");
         break;
      case BSON_TYPE_SYMBOL:
         break;
      case BSON_TYPE_EOD:
      case BSON_TYPE_UTF8:
      case BSON_TYPE_DOCUMENT:
      case BSON_TYPE_ARRAY:
      case BSON_TYPE_BOOL:
      case BSON_TYPE_NULL:
      case BSON_TYPE_TIMESTAMP:
      default:
         _bson_json_read_set_error (
            reader,
            "Internal error: can't parse JSON wrapper for type \"%s\"",
            _bson_json_type_name (bson->bson_type));
         break;
      }
   } else if (bson->read_state == BSON_JSON_IN_BSON_TYPE_TIMESTAMP_VALUES) {
      if (!bson->key) {
         _bad_extended_json (reader);
         return;
      }

      bson->read_state = BSON_JSON_IN_BSON_TYPE_TIMESTAMP_ENDMAP;
      _bson_json_read_append_timestamp (reader, bson);
      return;
   } else if (bson->read_state == BSON_JSON_IN_BSON_TYPE_REGEX_VALUES) {
      if (!bson->key) {
         _bad_extended_json (reader);
         return;
      }

      bson->read_state = BSON_JSON_IN_BSON_TYPE_REGEX_ENDMAP;
      _bson_json_read_append_regex (reader, bson);
      return;
   } else if (bson->read_state == BSON_JSON_IN_BSON_TYPE_BINARY_VALUES) {
      if (!bson->key) {
         _bad_extended_json (reader);
         return;
      }

      bson->read_state = BSON_JSON_IN_BSON_TYPE_BINARY_ENDMAP;
      _bson_json_read_append_binary (reader, bson);
      return;
   } else if (bson->read_state == BSON_JSON_IN_BSON_TYPE_TIMESTAMP_ENDMAP) {
      bson->read_state = BSON_JSON_REGULAR;
   } else if (bson->read_state == BSON_JSON_IN_BSON_TYPE_REGEX_ENDMAP) {
      bson->read_state = BSON_JSON_REGULAR;
   } else if (bson->read_state == BSON_JSON_IN_BSON_TYPE_BINARY_ENDMAP) {
      bson->read_state = BSON_JSON_REGULAR;
   } else if (bson->read_state == BSON_JSON_IN_BSON_TYPE_DATE_NUMBERLONG) {
      if (!bson->key) {
         _bad_extended_json (reader);
         return;
      }

      bson->read_state = BSON_JSON_IN_BSON_TYPE_DATE_ENDMAP;

      _bson_json_read_append_date_time (reader, bson);
      return;
   } else if (bson->read_state == BSON_JSON_IN_BSON_TYPE_DATE_ENDMAP) {
      bson->read_state = BSON_JSON_REGULAR;
   } else if (bson->read_state == BSON_JSON_REGULAR) {
      if (STACK_IS_SCOPE) {
         bson->read_state = BSON_JSON_IN_BSON_TYPE;
         bson->bson_type = BSON_TYPE_CODE;
         STACK_POP_SCOPE;
      } else if (STACK_IS_DBPOINTER) {
         bson->read_state = BSON_JSON_IN_BSON_TYPE_DBPOINTER_STARTMAP;
         STACK_POP_DBPOINTER;
      } else {
         if (STACK_HAS_ID != STACK_HAS_REF) {
            _bson_json_read_set_error (
               reader, "%s", "DBRef object must have both $ref and $id keys");
         }

         STACK_POP_DOC (
            bson_append_document_end (STACK_BSON_PARENT, STACK_BSON_CHILD));
      }

      if (bson->n == -1) {
         bson->read_state = BSON_JSON_DONE;
      }
   } else if (bson->read_state == BSON_JSON_IN_SCOPE) {
      /* empty $scope */
      BSON_ASSERT (bson->code_data.has_scope);
      STACK_PUSH_SCOPE (bson_init (STACK_BSON_CHILD));
      STACK_POP_SCOPE;
      bson->read_state = BSON_JSON_IN_BSON_TYPE;
      bson->bson_type = BSON_TYPE_CODE;
   } else if (bson->read_state == BSON_JSON_IN_DBPOINTER) {
      /* empty $dbPointer??? */
      _bson_json_read_set_error (reader, "Empty $dbPointer");
   } else {
      _bson_json_read_set_error (
         reader, "Invalid state \"%s\"", read_state_names[bson->read_state]);
   }
}


static void
_bson_json_read_start_array (bson_json_reader_t *reader) /* IN */
{
   const char *key;
   size_t len;
   bson_json_reader_bson_t *bson = &reader->bson;

   if (bson->n < 0) {
      STACK_PUSH_ARRAY (_noop ());
   } else {
      _bson_json_read_fixup_key (bson);
      key = bson->key;
      len = bson->key_buf.len;

      if (bson->read_state != BSON_JSON_REGULAR) {
         _bson_json_read_set_error (reader,
                                    "Invalid read of \"[\" in state \"%s\"",
                                    read_state_names[bson->read_state]);
         return;
      }

      STACK_PUSH_ARRAY (bson_append_array_begin (
         STACK_BSON_PARENT, key, (int) len, STACK_BSON_CHILD));
   }
}


static void
_bson_json_read_end_array (bson_json_reader_t *reader) /* IN */
{
   bson_json_reader_bson_t *bson = &reader->bson;

   if (bson->read_state != BSON_JSON_REGULAR) {
      _bson_json_read_set_error (reader,
                                 "Invalid read of \"]\" in state \"%s\"",
                                 read_state_names[bson->read_state]);
      return;
   }

   STACK_POP_ARRAY (
      bson_append_array_end (STACK_BSON_PARENT, STACK_BSON_CHILD));
   if (bson->n == -1) {
      bson->read_state = BSON_JSON_DONE;
   }
}


/* put unescaped text in reader->bson.unescaped, or set reader->error.
 * json_text has length len and it is not null-terminated. */
static bool
_bson_json_unescape (bson_json_reader_t *reader,
                     struct jsonsl_state_st *state,
                     const char *json_text,
                     ssize_t len)
{
   bson_json_reader_bson_t *reader_bson;
   jsonsl_error_t err;

   reader_bson = &reader->bson;

   /* add 1 for NULL */
   _bson_json_buf_ensure (&reader_bson->unescaped, (size_t) len + 1);

   /* length of unescaped str is always <= len */
   reader_bson->unescaped.len = jsonsl_util_unescape (
      json_text, (char *) reader_bson->unescaped.buf, (size_t) len, NULL, &err);

   if (err != JSONSL_ERROR_SUCCESS) {
      bson_set_error (reader->error,
                      BSON_ERROR_JSON,
                      BSON_JSON_ERROR_READ_CORRUPT_JS,
                      "error near position %d: \"%s\"",
                      (int) state->pos_begin,
                      jsonsl_strerror (err));
      return false;
   }

   reader_bson->unescaped.buf[reader_bson->unescaped.len] = '\0';

   return true;
}


/* read the buffered JSON plus new data, and fill out @len with its length */
static const char *
_get_json_text (jsonsl_t json,                 /* IN */
                struct jsonsl_state_st *state, /* IN */
                const char *buf /* IN */,
                ssize_t *len /* OUT */)
{
   bson_json_reader_t *reader;
   ssize_t bytes_available;

   reader = (bson_json_reader_t *) json->data;

   BSON_ASSERT (state->pos_cur > state->pos_begin);

   *len = (ssize_t) (state->pos_cur - state->pos_begin);

   bytes_available = buf - json->base;

   if (*len <= bytes_available) {
      /* read directly from stream, not from saved JSON */
      return buf - (size_t) *len;
   } else {
      /* combine saved text with new data from the jsonsl_t */
      ssize_t append = buf - json->base;

      if (append > 0) {
         _bson_json_buf_append (
            &reader->tok_accumulator, buf - append, (size_t) append);
      }

      return (const char *) reader->tok_accumulator.buf;
   }
}


static void
_push_callback (jsonsl_t json,
                jsonsl_action_t action,
                struct jsonsl_state_st *state,
                const char *buf)
{
   bson_json_reader_t *reader = (bson_json_reader_t *) json->data;

   switch (state->type) {
   case JSONSL_T_STRING:
   case JSONSL_T_HKEY:
   case JSONSL_T_SPECIAL:
   case JSONSL_T_UESCAPE:
      reader->json_text_pos = state->pos_begin;
      break;
   case JSONSL_T_OBJECT:
      _bson_json_read_start_map (reader);
      break;
   case JSONSL_T_LIST:
      _bson_json_read_start_array (reader);
      break;
   default:
      break;
   }
}


static void
_pop_callback (jsonsl_t json,
               jsonsl_action_t action,
               struct jsonsl_state_st *state,
               const char *buf)
{
   bson_json_reader_t *reader;
   bson_json_reader_bson_t *reader_bson;
   ssize_t len;
   double d;
   const char *obj_text;

   reader = (bson_json_reader_t *) json->data;
   reader_bson = &reader->bson;

   switch (state->type) {
   case JSONSL_T_HKEY:
   case JSONSL_T_STRING:
      obj_text = _get_json_text (json, state, buf, &len);
      BSON_ASSERT (obj_text[0] == '"');

      /* remove start/end quotes, replace backslash-escapes, null-terminate */
      /* you'd think it would be faster to check if state->nescapes > 0 first,
       * but tests show no improvement */
      if (!_bson_json_unescape (reader, state, obj_text + 1, len - 1)) {
         /* reader->error is set */
         jsonsl_stop (json);
         break;
      }

      if (state->type == JSONSL_T_HKEY) {
         _bson_json_read_map_key (
            reader, reader_bson->unescaped.buf, reader_bson->unescaped.len);
      } else {
         _bson_json_read_string (
            reader, reader_bson->unescaped.buf, reader_bson->unescaped.len);
      }
      break;
   case JSONSL_T_OBJECT:
      _bson_json_read_end_map (reader);
      break;
   case JSONSL_T_LIST:
      _bson_json_read_end_array (reader);
      break;
   case JSONSL_T_SPECIAL:
      obj_text = _get_json_text (json, state, buf, &len);
      if (state->special_flags & JSONSL_SPECIALf_NUMNOINT) {
         if (_bson_json_parse_double (reader, obj_text, (size_t) len, &d)) {
            _bson_json_read_double (reader, d);
         }
      } else if (state->special_flags & JSONSL_SPECIALf_NUMERIC) {
         /* jsonsl puts the unsigned value in state->nelem */
         _bson_json_read_integer (
            reader,
            state->nelem,
            state->special_flags & JSONSL_SPECIALf_SIGNED ? -1 : 1);
      } else if (state->special_flags & JSONSL_SPECIALf_BOOLEAN) {
         _bson_json_read_boolean (reader, obj_text[0] == 't' ? 1 : 0);
      } else if (state->special_flags & JSONSL_SPECIALf_NULL) {
         _bson_json_read_null (reader);
      }
      break;
   default:
      break;
   }

   reader->json_text_pos = -1;
   reader->tok_accumulator.len = 0;
}


static int
_error_callback (jsonsl_t json,
                 jsonsl_error_t err,
                 struct jsonsl_state_st *state,
                 char *errat)
{
   bson_json_reader_t *reader = (bson_json_reader_t *) json->data;

   if (err == JSONSL_ERROR_CANT_INSERT && *errat == '{') {
      /* start the next document */
      reader->should_reset = true;
      reader->advance = errat - json->base;
      return 0;
   } else if (err == JSONSL_ERROR_WEIRD_WHITESPACE && *errat == '\0') {
      /* embedded NULL is ok */
      json->pos++;
      return 1;
   }

   bson_set_error (reader->error,
                   BSON_ERROR_JSON,
                   BSON_JSON_ERROR_READ_CORRUPT_JS,
                   "Got parse error at \"%c\", position %d: \"%s\"",
                   *errat,
                   (int) json->pos,
                   jsonsl_strerror (err));

   return 0;
}


/*
 *--------------------------------------------------------------------------
 *
 * bson_json_reader_read --
 *
 *       Read the next json document from @reader and write its value
 *       into @bson. @bson will be allocated as part of this process.
 *
 *       @bson MUST be initialized before calling this function as it
 *       will not be initialized automatically. The reasoning for this
 *       is so that you can chain together bson_json_reader_t with
 *       other components like bson_writer_t.
 *
 * Returns:
 *       1 if successful and data was read.
 *       0 if successful and no data was read.
 *       -1 if there was an error and @error is set.
 *
 * Side effects:
 *       @error may be set.
 *
 *--------------------------------------------------------------------------
 */

int
bson_json_reader_read (bson_json_reader_t *reader, /* IN */
                       bson_t *bson,               /* IN */
                       bson_error_t *error)        /* OUT */
{
   bson_json_reader_producer_t *p;
   ssize_t start_pos;
   ssize_t r;
   ssize_t buf_offset;
   ssize_t accum;
   bson_error_t error_tmp;
   int ret = 0;

   BSON_ASSERT (reader);
   BSON_ASSERT (bson);

   p = &reader->producer;

   reader->bson.bson = bson;
   reader->bson.n = -1;
   reader->bson.read_state = BSON_JSON_REGULAR;
   reader->error = error ? error : &error_tmp;
   memset (reader->error, 0, sizeof (bson_error_t));

   for (;;) {
      start_pos = reader->json->pos;

      if (p->bytes_read > 0) {
         /* leftover data from previous JSON doc in the stream */
         r = p->bytes_read;
      } else {
         /* read a chunk of bytes by executing the callback */
         r = p->cb (p->data, p->buf, p->buf_size);
      }

      if (r < 0) {
         if (error) {
            bson_set_error (error,
                            BSON_ERROR_JSON,
                            BSON_JSON_ERROR_READ_CB_FAILURE,
                            "reader cb failed");
         }
         ret = -1;
         goto cleanup;
      } else if (r == 0) {
         break;
      } else {
         ret = 1;
         p->bytes_read = (size_t) r;

         jsonsl_feed (reader->json, (const jsonsl_char_t *) p->buf, (size_t) r);

         if (reader->should_reset) {
            /* end of a document */
            jsonsl_reset (reader->json);
            reader->should_reset = false;

            /* advance past already-parsed data */
            memmove (p->buf, p->buf + reader->advance, r - reader->advance);
            p->bytes_read -= reader->advance;
            ret = 1;
            goto cleanup;
         }

         if (reader->error->domain) {
            ret = -1;
            goto cleanup;
         }

         /* accumulate a key or string value */
         if (reader->json_text_pos != -1) {
            if (reader->json_text_pos < reader->json->pos) {
               accum = BSON_MIN (reader->json->pos - reader->json_text_pos, r);
               /* if this chunk stopped mid-token, buf_offset is how far into
                * our current chunk the token begins. */
               buf_offset = AT_LEAST_0 (reader->json_text_pos - start_pos);
               _bson_json_buf_append (&reader->tok_accumulator,
                                      p->buf + buf_offset,
                                      (size_t) accum);
            }
         }

         p->bytes_read = 0;
      }
   }

cleanup:
   if (ret == 1 && reader->bson.read_state != BSON_JSON_DONE) {
      /* data ended in the middle */
      _bson_json_read_corrupt (reader, "%s", "Incomplete JSON");
      return -1;
   }

   return ret;
}


bson_json_reader_t *
bson_json_reader_new (void *data,               /* IN */
                      bson_json_reader_cb cb,   /* IN */
                      bson_json_destroy_cb dcb, /* IN */
                      bool allow_multiple,      /* unused */
                      size_t buf_size)          /* IN */
{
   bson_json_reader_t *r;
   bson_json_reader_producer_t *p;

   r = bson_malloc0 (sizeof *r);
   r->json = jsonsl_new (STACK_MAX);
   r->json->error_callback = _error_callback;
   r->json->action_callback_PUSH = _push_callback;
   r->json->action_callback_POP = _pop_callback;
   r->json->data = r;
   r->json_text_pos = -1;
   jsonsl_enable_all_callbacks (r->json);

   p = &r->producer;

   p->data = data;
   p->cb = cb;
   p->dcb = dcb;
   p->buf_size = buf_size ? buf_size : BSON_JSON_DEFAULT_BUF_SIZE;
   p->buf = bson_malloc (p->buf_size);

   return r;
}


void
bson_json_reader_destroy (bson_json_reader_t *reader) /* IN */
{
   int i;
   bson_json_reader_producer_t *p = &reader->producer;
   bson_json_reader_bson_t *b = &reader->bson;

   if (reader->producer.dcb) {
      reader->producer.dcb (reader->producer.data);
   }

   bson_free (p->buf);
   bson_free (b->key_buf.buf);
   bson_free (b->unescaped.buf);
   bson_free (b->dbpointer_key.buf);

   for (i = 0; i < 3; i++) {
      bson_free (b->bson_type_buf[i].buf);
   }

   _bson_json_code_cleanup (&b->code_data);

   jsonsl_destroy (reader->json);
   bson_free (reader->tok_accumulator.buf);
   bson_free (reader);
}


typedef struct {
   const uint8_t *data;
   size_t len;
   size_t bytes_parsed;
} bson_json_data_reader_t;


static ssize_t
_bson_json_data_reader_cb (void *_ctx, uint8_t *buf, size_t len)
{
   size_t bytes;
   bson_json_data_reader_t *ctx = (bson_json_data_reader_t *) _ctx;

   if (!ctx->data) {
      return -1;
   }

   bytes = BSON_MIN (len, ctx->len - ctx->bytes_parsed);

   memcpy (buf, ctx->data + ctx->bytes_parsed, bytes);

   ctx->bytes_parsed += bytes;

   return bytes;
}


bson_json_reader_t *
bson_json_data_reader_new (bool allow_multiple, /* IN */
                           size_t size)         /* IN */
{
   bson_json_data_reader_t *dr = bson_malloc0 (sizeof *dr);

   return bson_json_reader_new (
      dr, &_bson_json_data_reader_cb, &bson_free, allow_multiple, size);
}


void
bson_json_data_reader_ingest (bson_json_reader_t *reader, /* IN */
                              const uint8_t *data,        /* IN */
                              size_t len)                 /* IN */
{
   bson_json_data_reader_t *ctx =
      (bson_json_data_reader_t *) reader->producer.data;

   ctx->data = data;
   ctx->len = len;
   ctx->bytes_parsed = 0;
}


bson_t *
bson_new_from_json (const uint8_t *data, /* IN */
                    ssize_t len,         /* IN */
                    bson_error_t *error) /* OUT */
{
   bson_json_reader_t *reader;
   bson_t *bson;
   int r;

   BSON_ASSERT (data);

   if (len < 0) {
      len = (ssize_t) strlen ((const char *) data);
   }

   bson = bson_new ();
   reader = bson_json_data_reader_new (false, BSON_JSON_DEFAULT_BUF_SIZE);
   bson_json_data_reader_ingest (reader, data, len);
   r = bson_json_reader_read (reader, bson, error);
   bson_json_reader_destroy (reader);

   if (r == 0) {
      bson_set_error (error,
                      BSON_ERROR_JSON,
                      BSON_JSON_ERROR_READ_INVALID_PARAM,
                      "Empty JSON string");
   }

   if (r != 1) {
      bson_destroy (bson);
      return NULL;
   }

   return bson;
}


bool
bson_init_from_json (bson_t *bson,        /* OUT */
                     const char *data,    /* IN */
                     ssize_t len,         /* IN */
                     bson_error_t *error) /* OUT */
{
   bson_json_reader_t *reader;
   int r;

   BSON_ASSERT (bson);
   BSON_ASSERT (data);

   if (len < 0) {
      len = strlen (data);
   }

   bson_init (bson);

   reader = bson_json_data_reader_new (false, BSON_JSON_DEFAULT_BUF_SIZE);
   bson_json_data_reader_ingest (reader, (const uint8_t *) data, len);
   r = bson_json_reader_read (reader, bson, error);
   bson_json_reader_destroy (reader);

   if (r == 0) {
      bson_set_error (error,
                      BSON_ERROR_JSON,
                      BSON_JSON_ERROR_READ_INVALID_PARAM,
                      "Empty JSON string");
   }

   if (r != 1) {
      bson_destroy (bson);
      return false;
   }

   return true;
}


static void
_bson_json_reader_handle_fd_destroy (void *handle) /* IN */
{
   bson_json_reader_handle_fd_t *fd = handle;

   if (fd) {
      if ((fd->fd != -1) && fd->do_close) {
#ifdef _WIN32
         _close (fd->fd);
#else
         close (fd->fd);
#endif
      }
      bson_free (fd);
   }
}


static ssize_t
_bson_json_reader_handle_fd_read (void *handle, /* IN */
                                  uint8_t *buf, /* IN */
                                  size_t len)   /* IN */
{
   bson_json_reader_handle_fd_t *fd = handle;
   ssize_t ret = -1;

   if (fd && (fd->fd != -1)) {
   again:
#ifdef BSON_OS_WIN32
      ret = _read (fd->fd, buf, (unsigned int) len);
#else
      ret = read (fd->fd, buf, len);
#endif
      if ((ret == -1) && (errno == EAGAIN)) {
         goto again;
      }
   }

   return ret;
}


bson_json_reader_t *
bson_json_reader_new_from_fd (int fd,                /* IN */
                              bool close_on_destroy) /* IN */
{
   bson_json_reader_handle_fd_t *handle;

   BSON_ASSERT (fd != -1);

   handle = bson_malloc0 (sizeof *handle);
   handle->fd = fd;
   handle->do_close = close_on_destroy;

   return bson_json_reader_new (handle,
                                _bson_json_reader_handle_fd_read,
                                _bson_json_reader_handle_fd_destroy,
                                true,
                                BSON_JSON_DEFAULT_BUF_SIZE);
}


bson_json_reader_t *
bson_json_reader_new_from_file (const char *path,    /* IN */
                                bson_error_t *error) /* OUT */
{
   char errmsg_buf[BSON_ERROR_BUFFER_SIZE];
   char *errmsg;
   int fd = -1;

   BSON_ASSERT (path);

#ifdef BSON_OS_WIN32
   _sopen_s (&fd, path, (_O_RDONLY | _O_BINARY), _SH_DENYNO, _S_IREAD);
#else
   fd = open (path, O_RDONLY);
#endif

   if (fd == -1) {
      errmsg = bson_strerror_r (errno, errmsg_buf, sizeof errmsg_buf);
      bson_set_error (
         error, BSON_ERROR_READER, BSON_ERROR_READER_BADFD, "%s", errmsg);
      return NULL;
   }

   return bson_json_reader_new_from_fd (fd, true);
}
