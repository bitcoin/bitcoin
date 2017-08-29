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


#include "bson-memory.h"
#include "bson-string.h"
#include "bson-value.h"
#include "bson-oid.h"


void
bson_value_copy (const bson_value_t *src, /* IN */
                 bson_value_t *dst)       /* OUT */
{
   BSON_ASSERT (src);
   BSON_ASSERT (dst);

   dst->value_type = src->value_type;

   switch (src->value_type) {
   case BSON_TYPE_DOUBLE:
      dst->value.v_double = src->value.v_double;
      break;
   case BSON_TYPE_UTF8:
      dst->value.v_utf8.len = src->value.v_utf8.len;
      dst->value.v_utf8.str = bson_malloc (src->value.v_utf8.len + 1);
      memcpy (
         dst->value.v_utf8.str, src->value.v_utf8.str, dst->value.v_utf8.len);
      dst->value.v_utf8.str[dst->value.v_utf8.len] = '\0';
      break;
   case BSON_TYPE_DOCUMENT:
   case BSON_TYPE_ARRAY:
      dst->value.v_doc.data_len = src->value.v_doc.data_len;
      dst->value.v_doc.data = bson_malloc (src->value.v_doc.data_len);
      memcpy (dst->value.v_doc.data,
              src->value.v_doc.data,
              dst->value.v_doc.data_len);
      break;
   case BSON_TYPE_BINARY:
      dst->value.v_binary.subtype = src->value.v_binary.subtype;
      dst->value.v_binary.data_len = src->value.v_binary.data_len;
      dst->value.v_binary.data = bson_malloc (src->value.v_binary.data_len);
      memcpy (dst->value.v_binary.data,
              src->value.v_binary.data,
              dst->value.v_binary.data_len);
      break;
   case BSON_TYPE_OID:
      bson_oid_copy (&src->value.v_oid, &dst->value.v_oid);
      break;
   case BSON_TYPE_BOOL:
      dst->value.v_bool = src->value.v_bool;
      break;
   case BSON_TYPE_DATE_TIME:
      dst->value.v_datetime = src->value.v_datetime;
      break;
   case BSON_TYPE_REGEX:
      dst->value.v_regex.regex = bson_strdup (src->value.v_regex.regex);
      dst->value.v_regex.options = bson_strdup (src->value.v_regex.options);
      break;
   case BSON_TYPE_DBPOINTER:
      dst->value.v_dbpointer.collection_len =
         src->value.v_dbpointer.collection_len;
      dst->value.v_dbpointer.collection =
         bson_malloc (src->value.v_dbpointer.collection_len + 1);
      memcpy (dst->value.v_dbpointer.collection,
              src->value.v_dbpointer.collection,
              dst->value.v_dbpointer.collection_len);
      dst->value.v_dbpointer.collection[dst->value.v_dbpointer.collection_len] =
         '\0';
      bson_oid_copy (&src->value.v_dbpointer.oid, &dst->value.v_dbpointer.oid);
      break;
   case BSON_TYPE_CODE:
      dst->value.v_code.code_len = src->value.v_code.code_len;
      dst->value.v_code.code = bson_malloc (src->value.v_code.code_len + 1);
      memcpy (dst->value.v_code.code,
              src->value.v_code.code,
              dst->value.v_code.code_len);
      dst->value.v_code.code[dst->value.v_code.code_len] = '\0';
      break;
   case BSON_TYPE_SYMBOL:
      dst->value.v_symbol.len = src->value.v_symbol.len;
      dst->value.v_symbol.symbol = bson_malloc (src->value.v_symbol.len + 1);
      memcpy (dst->value.v_symbol.symbol,
              src->value.v_symbol.symbol,
              dst->value.v_symbol.len);
      dst->value.v_symbol.symbol[dst->value.v_symbol.len] = '\0';
      break;
   case BSON_TYPE_CODEWSCOPE:
      dst->value.v_codewscope.code_len = src->value.v_codewscope.code_len;
      dst->value.v_codewscope.code =
         bson_malloc (src->value.v_codewscope.code_len + 1);
      memcpy (dst->value.v_codewscope.code,
              src->value.v_codewscope.code,
              dst->value.v_codewscope.code_len);
      dst->value.v_codewscope.code[dst->value.v_codewscope.code_len] = '\0';
      dst->value.v_codewscope.scope_len = src->value.v_codewscope.scope_len;
      dst->value.v_codewscope.scope_data =
         bson_malloc (src->value.v_codewscope.scope_len);
      memcpy (dst->value.v_codewscope.scope_data,
              src->value.v_codewscope.scope_data,
              dst->value.v_codewscope.scope_len);
      break;
   case BSON_TYPE_INT32:
      dst->value.v_int32 = src->value.v_int32;
      break;
   case BSON_TYPE_TIMESTAMP:
      dst->value.v_timestamp.timestamp = src->value.v_timestamp.timestamp;
      dst->value.v_timestamp.increment = src->value.v_timestamp.increment;
      break;
   case BSON_TYPE_INT64:
      dst->value.v_int64 = src->value.v_int64;
      break;
   case BSON_TYPE_DECIMAL128:
      dst->value.v_decimal128 = src->value.v_decimal128;
      break;
   case BSON_TYPE_UNDEFINED:
   case BSON_TYPE_NULL:
   case BSON_TYPE_MAXKEY:
   case BSON_TYPE_MINKEY:
      break;
   case BSON_TYPE_EOD:
   default:
      BSON_ASSERT (false);
      return;
   }
}


void
bson_value_destroy (bson_value_t *value) /* IN */
{
   switch (value->value_type) {
   case BSON_TYPE_UTF8:
      bson_free (value->value.v_utf8.str);
      break;
   case BSON_TYPE_DOCUMENT:
   case BSON_TYPE_ARRAY:
      bson_free (value->value.v_doc.data);
      break;
   case BSON_TYPE_BINARY:
      bson_free (value->value.v_binary.data);
      break;
   case BSON_TYPE_REGEX:
      bson_free (value->value.v_regex.regex);
      bson_free (value->value.v_regex.options);
      break;
   case BSON_TYPE_DBPOINTER:
      bson_free (value->value.v_dbpointer.collection);
      break;
   case BSON_TYPE_CODE:
      bson_free (value->value.v_code.code);
      break;
   case BSON_TYPE_SYMBOL:
      bson_free (value->value.v_symbol.symbol);
      break;
   case BSON_TYPE_CODEWSCOPE:
      bson_free (value->value.v_codewscope.code);
      bson_free (value->value.v_codewscope.scope_data);
      break;
   case BSON_TYPE_DOUBLE:
   case BSON_TYPE_UNDEFINED:
   case BSON_TYPE_OID:
   case BSON_TYPE_BOOL:
   case BSON_TYPE_DATE_TIME:
   case BSON_TYPE_NULL:
   case BSON_TYPE_INT32:
   case BSON_TYPE_TIMESTAMP:
   case BSON_TYPE_INT64:
   case BSON_TYPE_DECIMAL128:
   case BSON_TYPE_MAXKEY:
   case BSON_TYPE_MINKEY:
   case BSON_TYPE_EOD:
   default:
      break;
   }
}
