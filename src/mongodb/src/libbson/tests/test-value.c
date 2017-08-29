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


#include <bcon.h>
#include <bson.h>

#include "TestSuite.h"


static void
test_value_basic (void)
{
   static const uint8_t raw[16] = {0};
   const bson_value_t *value;
   bson_value_t copy;
   bson_iter_t iter;
   bson_oid_t oid;
   bson_t other = BSON_INITIALIZER;
   bson_t *doc;
   bson_t sub = BSON_INITIALIZER;
   bool r;
   int i;

   bson_oid_init (&oid, NULL);

   doc = BCON_NEW ("double",
                   BCON_DOUBLE (123.4),
                   "utf8",
                   "this is my string",
                   "document",
                   BCON_DOCUMENT (&sub),
                   "array",
                   BCON_DOCUMENT (&sub),
                   "binary",
                   BCON_BIN (BSON_SUBTYPE_BINARY, raw, sizeof raw),
                   "undefined",
                   BCON_UNDEFINED,
                   "oid",
                   BCON_OID (&oid),
                   "bool",
                   BCON_BOOL (true),
                   "datetime",
                   BCON_DATE_TIME (12345678),
                   "null",
                   BCON_NULL,
                   "regex",
                   BCON_REGEX ("^hello", "i"),
                   "dbpointer",
                   BCON_DBPOINTER ("test.test", &oid),
                   "code",
                   BCON_CODE ("var a = function() {}"),
                   "symbol",
                   BCON_SYMBOL ("my_symbol"),
                   "codewscope",
                   BCON_CODEWSCOPE ("var a = 1;", &sub),
                   "int32",
                   BCON_INT32 (1234),
                   "timestamp",
                   BCON_TIMESTAMP (1234, 4567),
                   "int64",
                   BCON_INT32 (4321),
                   "maxkey",
                   BCON_MAXKEY,
                   "minkey",
                   BCON_MINKEY);

   r = bson_iter_init (&iter, doc);
   BSON_ASSERT (r);

   for (i = 0; i < 20; i++) {
      r = bson_iter_next (&iter);
      BSON_ASSERT (r);

      value = bson_iter_value (&iter);
      BSON_ASSERT (value);

      bson_value_copy (value, &copy);

      r = bson_append_value (&other, bson_iter_key (&iter), -1, &copy);
      BSON_ASSERT (r);

      bson_value_destroy (&copy);
   }

   r = bson_iter_next (&iter);
   BSON_ASSERT (!r);

   bson_destroy (doc);
   bson_destroy (&other);
}


static void
test_value_decimal128 (void)
{
   const bson_value_t *value;
   bson_value_t copy;
   bson_iter_t iter;
   bson_decimal128_t dec;
   bson_t other = BSON_INITIALIZER;
   bson_t *doc;

   BSON_ASSERT (bson_decimal128_from_string ("123.5", &dec));
   doc = BCON_NEW ("decimal128", BCON_DECIMAL128 (&dec));
   BSON_ASSERT (bson_iter_init (&iter, doc) && bson_iter_next (&iter));
   BSON_ASSERT ((value = bson_iter_value (&iter)));
   bson_value_copy (value, &copy);
   BSON_ASSERT (bson_append_value (&other, bson_iter_key (&iter), -1, &copy));

   bson_value_destroy (&copy);
   bson_destroy (doc);
   bson_destroy (&other);
}


void
test_value_install (TestSuite *suite)
{
   TestSuite_Add (suite, "/bson/value/basic", test_value_basic);
   TestSuite_Add (suite, "/bson/value/decimal128", test_value_decimal128);
}
