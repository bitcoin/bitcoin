/*
 * Copyright 2013 MongoDB, Inc.
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


#include <bson.h>
#include <bcon.h>
#define BSON_INSIDE
#include <bson-private.h>
#undef BSON_INSIDE
#include <fcntl.h>
#include <time.h>

#include "bson-tests.h"
#include "TestSuite.h"

#ifndef BINARY_DIR
#define BINARY_DIR "tests/binary"
#endif


static bson_t *
get_bson (const char *filename)
{
   ssize_t len;
   uint8_t buf[4096];
   bson_t *b;
   char real_filename[256];
   int fd;

   bson_snprintf (
      real_filename, sizeof real_filename, BINARY_DIR "/%s", filename);
   real_filename[sizeof real_filename - 1] = '\0';

   if (-1 == (fd = bson_open (real_filename, O_RDONLY))) {
      fprintf (stderr, "Failed to bson_open: %s\n", real_filename);
      abort ();
   }
   len = bson_read (fd, buf, sizeof buf);
   BSON_ASSERT (len > 0);
   b = bson_new_from_data (buf, (uint32_t) len);
   bson_close (fd);

   return b;
}


static void
test_bson_new (void)
{
   bson_t *b;

   b = bson_new ();
   BSON_ASSERT_CMPINT (b->len, ==, 5);
   bson_destroy (b);

   b = bson_sized_new (32);
   BSON_ASSERT_CMPINT (b->len, ==, 5);
   bson_destroy (b);
}


static void
test_bson_alloc (void)
{
   static const uint8_t empty_bson[] = {5, 0, 0, 0, 0};
   bson_t *b;

   b = bson_new ();
   BSON_ASSERT_CMPINT (b->len, ==, 5);
   BSON_ASSERT ((b->flags & BSON_FLAG_INLINE));
   BSON_ASSERT (!(b->flags & BSON_FLAG_CHILD));
   BSON_ASSERT (!(b->flags & BSON_FLAG_STATIC));
   BSON_ASSERT (!(b->flags & BSON_FLAG_NO_FREE));
   bson_destroy (b);

   /*
    * This checks that we fit in the inline buffer size.
    */
   b = bson_sized_new (44);
   BSON_ASSERT_CMPINT (b->len, ==, 5);
   BSON_ASSERT ((b->flags & BSON_FLAG_INLINE));
   bson_destroy (b);

   /*
    * Make sure we grow to next power of 2.
    */
   b = bson_sized_new (121);
   BSON_ASSERT_CMPINT (b->len, ==, 5);
   BSON_ASSERT (!(b->flags & BSON_FLAG_INLINE));
   bson_destroy (b);

   /*
    * Make sure we grow to next power of 2.
    */
   b = bson_sized_new (129);
   BSON_ASSERT_CMPINT (b->len, ==, 5);
   BSON_ASSERT (!(b->flags & BSON_FLAG_INLINE));
   bson_destroy (b);

   b = bson_new_from_data (empty_bson, sizeof empty_bson);
   BSON_ASSERT_CMPINT (b->len, ==, sizeof empty_bson);
   BSON_ASSERT ((b->flags & BSON_FLAG_INLINE));
   BSON_ASSERT (!memcmp (bson_get_data (b), empty_bson, sizeof empty_bson));
   bson_destroy (b);
}


static void
BSON_ASSERT_BSON_EQUAL (const bson_t *a, const bson_t *b)
{
   const uint8_t *data1 = bson_get_data (a);
   const uint8_t *data2 = bson_get_data (b);
   uint32_t i;

   if (!bson_equal (a, b)) {
      for (i = 0; i < BSON_MAX (a->len, b->len); i++) {
         if (i >= a->len) {
            printf ("a is too short len=%u\n", a->len);
            abort ();
         } else if (i >= b->len) {
            printf ("b is too short len=%u\n", b->len);
            abort ();
         }
         if (data1[i] != data2[i]) {
            printf ("a[%u](0x%02x,%u) != b[%u](0x%02x,%u)\n",
                    i,
                    data1[i],
                    data1[i],
                    i,
                    data2[i],
                    data2[i]);
            abort ();
         }
      }
   }
}


static void
BSON_ASSERT_BSON_EQUAL_FILE (const bson_t *b, const char *filename)
{
   bson_t *b2 = get_bson (filename);
   BSON_ASSERT_BSON_EQUAL (b, b2);
   bson_destroy (b2);
}


static void
test_bson_append_utf8 (void)
{
   bson_t *b;
   bson_t *b2;

   b = bson_new ();
   b2 = get_bson ("test11.bson");
   BSON_ASSERT (bson_append_utf8 (b, "hello", -1, "world", -1));
   BSON_ASSERT_BSON_EQUAL (b, b2);
   bson_destroy (b);
   bson_destroy (b2);
}


static void
test_bson_append_symbol (void)
{
   bson_t *b;
   bson_t *b2;

   b = bson_new ();
   b2 = get_bson ("test32.bson");
   BSON_ASSERT (bson_append_symbol (b, "hello", -1, "world", -1));
   BSON_ASSERT_BSON_EQUAL (b, b2);
   bson_destroy (b);
   bson_destroy (b2);
}


static void
test_bson_append_null (void)
{
   bson_t *b;
   bson_t *b2;

   b = bson_new ();
   BSON_ASSERT (bson_append_null (b, "hello", -1));
   b2 = get_bson ("test18.bson");
   BSON_ASSERT_BSON_EQUAL (b, b2);
   bson_destroy (b);
   bson_destroy (b2);
}


static void
test_bson_append_bool (void)
{
   bson_t *b;
   bson_t *b2;

   b = bson_new ();
   BSON_ASSERT (bson_append_bool (b, "bool", -1, true));
   b2 = get_bson ("test19.bson");
   BSON_ASSERT_BSON_EQUAL (b, b2);
   bson_destroy (b);
   bson_destroy (b2);
}


static void
test_bson_append_double (void)
{
   bson_t *b;
   bson_t *b2;

   b = bson_new ();
   BSON_ASSERT (bson_append_double (b, "double", -1, 123.4567));
   b2 = get_bson ("test20.bson");
   BSON_ASSERT_BSON_EQUAL (b, b2);
   bson_destroy (b);
   bson_destroy (b2);
}


static void
test_bson_append_document (void)
{
   bson_t *b;
   bson_t *b2;

   b = bson_new ();
   b2 = bson_new ();
   BSON_ASSERT (bson_append_document (b, "document", -1, b2));
   bson_destroy (b2);
   b2 = get_bson ("test21.bson");
   BSON_ASSERT_BSON_EQUAL (b, b2);
   bson_destroy (b);
   bson_destroy (b2);
}


static void
test_bson_append_oid (void)
{
   bson_oid_t oid;
   bson_t *b;
   bson_t *b2;

   bson_oid_init_from_string (&oid, "1234567890abcdef1234abcd");

   b = bson_new ();
   BSON_ASSERT (bson_append_oid (b, "oid", -1, &oid));
   b2 = get_bson ("test22.bson");
   BSON_ASSERT_BSON_EQUAL (b, b2);
   bson_destroy (b);
   bson_destroy (b2);
}


static void
test_bson_append_array (void)
{
   bson_t *b;
   bson_t *b2;

   b = bson_new ();
   b2 = bson_new ();
   BSON_ASSERT (bson_append_utf8 (b2, "0", -1, "hello", -1));
   BSON_ASSERT (bson_append_utf8 (b2, "1", -1, "world", -1));
   BSON_ASSERT (bson_append_array (b, "array", -1, b2));
   bson_destroy (b2);
   b2 = get_bson ("test23.bson");
   BSON_ASSERT_BSON_EQUAL (b, b2);
   bson_destroy (b);
   bson_destroy (b2);
}


static void
test_bson_append_binary (void)
{
   const static uint8_t binary[] = {'1', '2', '3', '4'};
   bson_t *b;
   bson_t *b2;

   b = bson_new ();
   BSON_ASSERT (
      bson_append_binary (b, "binary", -1, BSON_SUBTYPE_USER, binary, 4));
   b2 = get_bson ("test24.bson");
   BSON_ASSERT_BSON_EQUAL (b, b2);
   bson_destroy (b);
   bson_destroy (b2);
}


static void
test_bson_append_binary_deprecated (void)
{
   const static uint8_t binary[] = {'1', '2', '3', '4'};
   bson_t *b;
   bson_t *b2;

   b = bson_new ();
   BSON_ASSERT (bson_append_binary (
      b, "binary", -1, BSON_SUBTYPE_BINARY_DEPRECATED, binary, 4));
   b2 = get_bson ("binary_deprecated.bson");
   BSON_ASSERT_BSON_EQUAL (b, b2);
   bson_destroy (b);
   bson_destroy (b2);
}


static void
test_bson_append_time_t (void)
{
   bson_t *b;
   bson_t *b2;
   time_t t;

   t = 1234567890;

   b = bson_new ();
   BSON_ASSERT (bson_append_time_t (b, "time_t", -1, t));
   b2 = get_bson ("test26.bson");
   BSON_ASSERT_BSON_EQUAL (b, b2);
   bson_destroy (b);
   bson_destroy (b2);
}


static void
test_bson_append_timeval (void)
{
   struct timeval tv = {0};
   bson_t *b;
   bson_t *b2;

   tv.tv_sec = 1234567890;
   tv.tv_usec = 0;

   b = bson_new ();
   BSON_ASSERT (bson_append_timeval (b, "time_t", -1, &tv));
   b2 = get_bson ("test26.bson");
   BSON_ASSERT_BSON_EQUAL (b, b2);
   bson_destroy (b);
   bson_destroy (b2);
}


static void
test_bson_append_undefined (void)
{
   bson_t *b;
   bson_t *b2;

   b = bson_new ();
   BSON_ASSERT (bson_append_undefined (b, "undefined", -1));
   b2 = get_bson ("test25.bson");
   BSON_ASSERT_BSON_EQUAL (b, b2);
   bson_destroy (b);
   bson_destroy (b2);
}


static void
test_bson_append_regex (void)
{
   bson_t *b;
   bson_t *b2;

   b = bson_new ();
   BSON_ASSERT (bson_append_regex (b, "regex", -1, "^abcd", "ilx"));
   b2 = get_bson ("test27.bson");
   BSON_ASSERT_BSON_EQUAL (b, b2);
   bson_destroy (b);
   bson_destroy (b2);
}


static void
test_bson_append_code (void)
{
   bson_t *b;
   bson_t *b2;

   b = bson_new ();
   BSON_ASSERT (bson_append_code (b, "code", -1, "var a = {};"));
   b2 = get_bson ("test29.bson");
   BSON_ASSERT_BSON_EQUAL (b, b2);
   bson_destroy (b);
   bson_destroy (b2);
}


static void
test_bson_append_code_with_scope (void)
{
   const uint8_t *scope_buf = NULL;
   uint32_t scopelen = 0;
   uint32_t len = 0;
   bson_iter_t iter;
   bool r;
   const char *code = NULL;
   bson_t *b;
   bson_t *b2;
   bson_t *scope;

   /* Test with NULL bson, which converts to just CODE type. */
   b = bson_new ();
   BSON_ASSERT (
      bson_append_code_with_scope (b, "code", -1, "var a = {};", NULL));
   b2 = get_bson ("test30.bson");
   BSON_ASSERT_BSON_EQUAL (b, b2);
   r = bson_iter_init_find (&iter, b, "code");
   BSON_ASSERT (r);
   BSON_ASSERT (BSON_ITER_HOLDS_CODE (&iter)); /* Not codewscope */
   bson_destroy (b);
   bson_destroy (b2);

   /* Empty scope is still CODEWSCOPE. */
   b = bson_new ();
   scope = bson_new ();
   BSON_ASSERT (
      bson_append_code_with_scope (b, "code", -1, "var a = {};", scope));
   b2 = get_bson ("code_w_empty_scope.bson");
   BSON_ASSERT_BSON_EQUAL (b, b2);
   r = bson_iter_init_find (&iter, b, "code");
   BSON_ASSERT (r);
   BSON_ASSERT (BSON_ITER_HOLDS_CODEWSCOPE (&iter));
   bson_destroy (b);
   bson_destroy (b2);
   bson_destroy (scope);

   /* Test with non-empty scope */
   b = bson_new ();
   scope = bson_new ();
   BSON_ASSERT (bson_append_utf8 (scope, "foo", -1, "bar", -1));
   BSON_ASSERT (
      bson_append_code_with_scope (b, "code", -1, "var a = {};", scope));
   b2 = get_bson ("test31.bson");
   BSON_ASSERT_BSON_EQUAL (b, b2);
   r = bson_iter_init_find (&iter, b, "code");
   BSON_ASSERT (r);
   BSON_ASSERT (BSON_ITER_HOLDS_CODEWSCOPE (&iter));
   code = bson_iter_codewscope (&iter, &len, &scopelen, &scope_buf);
   BSON_ASSERT (len == 11);
   BSON_ASSERT (scopelen == scope->len);
   BSON_ASSERT (!strcmp (code, "var a = {};"));
   bson_destroy (b);
   bson_destroy (b2);
   bson_destroy (scope);
}


static void
test_bson_append_dbpointer (void)
{
   bson_oid_t oid;
   bson_t *b;
   bson_t *b2;

   b = bson_new ();
   bson_oid_init_from_string (&oid, "0123abcd0123abcd0123abcd");
   BSON_ASSERT (bson_append_dbpointer (b, "dbpointer", -1, "foo", &oid));
   b2 = get_bson ("test28.bson");
   BSON_ASSERT_BSON_EQUAL (b, b2);
   bson_destroy (b);
   bson_destroy (b2);
}


static void
test_bson_append_int32 (void)
{
   bson_t *b;
   bson_t *b2;

   b = bson_new ();
   BSON_ASSERT (bson_append_int32 (b, "a", -1, -123));
   BSON_ASSERT (bson_append_int32 (b, "c", -1, 0));
   BSON_ASSERT (bson_append_int32 (b, "b", -1, 123));
   b2 = get_bson ("test33.bson");
   BSON_ASSERT_BSON_EQUAL (b, b2);
   bson_destroy (b);
   bson_destroy (b2);
}


static void
test_bson_append_int64 (void)
{
   bson_t *b;
   bson_t *b2;

   b = bson_new ();
   BSON_ASSERT (bson_append_int64 (b, "a", -1, 100000000000000ULL));
   b2 = get_bson ("test34.bson");
   BSON_ASSERT_BSON_EQUAL (b, b2);
   bson_destroy (b);
   bson_destroy (b2);
}


static void
test_bson_append_decimal128 (void)
{
   bson_t *b;
   bson_t *b2;
   bson_decimal128_t value;
   value.high = 0;
   value.low = 1;

   b = bson_new ();
   BSON_ASSERT (bson_append_decimal128 (b, "a", -1, &value));
   b2 = get_bson ("test58.bson");
   BSON_ASSERT_BSON_EQUAL (b, b2);
   bson_destroy (b);
   bson_destroy (b2);
}


static void
test_bson_append_iter (void)
{
   bson_iter_t iter;
   bool r;
   bson_t b;
   bson_t c;

   bson_init (&b);
   bson_append_int32 (&b, "a", 1, 1);
   bson_append_int32 (&b, "b", 1, 2);
   bson_append_int32 (&b, "c", 1, 3);
   bson_append_utf8 (&b, "d", 1, "hello", 5);

   bson_init (&c);

   r = bson_iter_init_find (&iter, &b, "a");
   BSON_ASSERT (r);
   r = bson_append_iter (&c, NULL, 0, &iter);
   BSON_ASSERT (r);

   r = bson_iter_init_find (&iter, &b, "c");
   BSON_ASSERT (r);
   r = bson_append_iter (&c, NULL, 0, &iter);
   BSON_ASSERT (r);

   r = bson_iter_init_find (&iter, &b, "d");
   BSON_ASSERT (r);
   r = bson_append_iter (&c, "world", -1, &iter);
   BSON_ASSERT (r);

   bson_iter_init (&iter, &c);
   r = bson_iter_next (&iter);
   BSON_ASSERT (r);
   BSON_ASSERT_CMPSTR ("a", bson_iter_key (&iter));
   BSON_ASSERT_CMPINT (BSON_TYPE_INT32, ==, bson_iter_type (&iter));
   BSON_ASSERT_CMPINT (1, ==, bson_iter_int32 (&iter));
   r = bson_iter_next (&iter);
   BSON_ASSERT (r);
   BSON_ASSERT_CMPSTR ("c", bson_iter_key (&iter));
   BSON_ASSERT_CMPINT (BSON_TYPE_INT32, ==, bson_iter_type (&iter));
   BSON_ASSERT_CMPINT (3, ==, bson_iter_int32 (&iter));
   r = bson_iter_next (&iter);
   BSON_ASSERT (r);
   BSON_ASSERT_CMPINT (BSON_TYPE_UTF8, ==, bson_iter_type (&iter));
   BSON_ASSERT_CMPSTR ("world", bson_iter_key (&iter));
   BSON_ASSERT_CMPSTR ("hello", bson_iter_utf8 (&iter, NULL));

   bson_destroy (&b);
   bson_destroy (&c);
}


static void
test_bson_append_timestamp (void)
{
   bson_t *b;
   bson_t *b2;

   b = bson_new ();
   BSON_ASSERT (bson_append_timestamp (b, "timestamp", -1, 1234, 9876));
   b2 = get_bson ("test35.bson");
   BSON_ASSERT_BSON_EQUAL (b, b2);
   bson_destroy (b);
   bson_destroy (b2);
}


static void
test_bson_append_maxkey (void)
{
   bson_t *b;
   bson_t *b2;

   b = bson_new ();
   BSON_ASSERT (bson_append_maxkey (b, "maxkey", -1));
   b2 = get_bson ("test37.bson");
   BSON_ASSERT_BSON_EQUAL (b, b2);
   bson_destroy (b);
   bson_destroy (b2);
}


static void
test_bson_append_minkey (void)
{
   bson_t *b;
   bson_t *b2;

   b = bson_new ();
   BSON_ASSERT (bson_append_minkey (b, "minkey", -1));
   b2 = get_bson ("test36.bson");
   BSON_ASSERT_BSON_EQUAL (b, b2);
   bson_destroy (b);
   bson_destroy (b2);
}


static void
test_bson_append_general (void)
{
   uint8_t bytes[12] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0x01, 0x23, 0x45};
   bson_oid_t oid;
   bson_t *bson;
   bson_t *array;
   bson_t *subdoc;

   bson = bson_new ();
   BSON_ASSERT (bson_append_int32 (bson, "int", -1, 1));
   BSON_ASSERT_BSON_EQUAL_FILE (bson, "test1.bson");
   bson_destroy (bson);

   bson = bson_new ();
   BSON_ASSERT (bson_append_int64 (bson, "int64", -1, 1));
   BSON_ASSERT_BSON_EQUAL_FILE (bson, "test2.bson");
   bson_destroy (bson);

   bson = bson_new ();
   BSON_ASSERT (bson_append_double (bson, "double", -1, 1.123));
   BSON_ASSERT_BSON_EQUAL_FILE (bson, "test3.bson");
   bson_destroy (bson);

   bson = bson_new ();
   BSON_ASSERT (bson_append_utf8 (bson, "string", -1, "some string", -1));
   BSON_ASSERT_BSON_EQUAL_FILE (bson, "test5.bson");
   bson_destroy (bson);

   bson = bson_new ();
   array = bson_new ();
   BSON_ASSERT (bson_append_int32 (array, "0", -1, 1));
   BSON_ASSERT (bson_append_int32 (array, "1", -1, 2));
   BSON_ASSERT (bson_append_int32 (array, "2", -1, 3));
   BSON_ASSERT (bson_append_int32 (array, "3", -1, 4));
   BSON_ASSERT (bson_append_int32 (array, "4", -1, 5));
   BSON_ASSERT (bson_append_int32 (array, "5", -1, 6));
   BSON_ASSERT (bson_append_array (bson, "array[int]", -1, array));
   BSON_ASSERT_BSON_EQUAL_FILE (bson, "test6.bson");
   bson_destroy (array);
   bson_destroy (bson);

   bson = bson_new ();
   array = bson_new ();
   BSON_ASSERT (bson_append_double (array, "0", -1, 1.123));
   BSON_ASSERT (bson_append_double (array, "1", -1, 2.123));
   BSON_ASSERT (bson_append_array (bson, "array[double]", -1, array));
   BSON_ASSERT_BSON_EQUAL_FILE (bson, "test7.bson");
   bson_destroy (array);
   bson_destroy (bson);

   bson = bson_new ();
   subdoc = bson_new ();
   BSON_ASSERT (bson_append_int32 (subdoc, "int", -1, 1));
   BSON_ASSERT (bson_append_document (bson, "document", -1, subdoc));
   BSON_ASSERT_BSON_EQUAL_FILE (bson, "test8.bson");
   bson_destroy (subdoc);
   bson_destroy (bson);

   bson = bson_new ();
   BSON_ASSERT (bson_append_null (bson, "null", -1));
   BSON_ASSERT_BSON_EQUAL_FILE (bson, "test9.bson");
   bson_destroy (bson);

   bson = bson_new ();
   BSON_ASSERT (bson_append_regex (bson, "regex", -1, "1234", "i"));
   BSON_ASSERT_BSON_EQUAL_FILE (bson, "test10.bson");
   bson_destroy (bson);

   bson = bson_new ();
   BSON_ASSERT (bson_append_utf8 (bson, "hello", -1, "world", -1));
   BSON_ASSERT_BSON_EQUAL_FILE (bson, "test11.bson");
   bson_destroy (bson);

   bson = bson_new ();
   array = bson_new ();
   BSON_ASSERT (bson_append_utf8 (array, "0", -1, "awesome", -1));
   BSON_ASSERT (bson_append_double (array, "1", -1, 5.05));
   BSON_ASSERT (bson_append_int32 (array, "2", -1, 1986));
   BSON_ASSERT (bson_append_array (bson, "BSON", -1, array));
   BSON_ASSERT_BSON_EQUAL_FILE (bson, "test12.bson");
   bson_destroy (bson);
   bson_destroy (array);

   bson = bson_new ();
   memcpy (&oid, bytes, sizeof oid);
   BSON_ASSERT (bson_append_oid (bson, "_id", -1, &oid));
   subdoc = bson_new ();
   BSON_ASSERT (bson_append_oid (subdoc, "_id", -1, &oid));
   array = bson_new ();
   BSON_ASSERT (bson_append_utf8 (array, "0", -1, "1", -1));
   BSON_ASSERT (bson_append_utf8 (array, "1", -1, "2", -1));
   BSON_ASSERT (bson_append_utf8 (array, "2", -1, "3", -1));
   BSON_ASSERT (bson_append_utf8 (array, "3", -1, "4", -1));
   BSON_ASSERT (bson_append_array (subdoc, "tags", -1, array));
   bson_destroy (array);
   BSON_ASSERT (bson_append_utf8 (subdoc, "text", -1, "asdfanother", -1));
   array = bson_new ();
   BSON_ASSERT (bson_append_utf8 (array, "name", -1, "blah", -1));
   BSON_ASSERT (bson_append_document (subdoc, "source", -1, array));
   bson_destroy (array);
   BSON_ASSERT (bson_append_document (bson, "document", -1, subdoc));
   bson_destroy (subdoc);
   array = bson_new ();
   BSON_ASSERT (bson_append_utf8 (array, "0", -1, "source", -1));
   BSON_ASSERT (bson_append_array (bson, "type", -1, array));
   bson_destroy (array);
   array = bson_new ();
   BSON_ASSERT (bson_append_utf8 (array, "0", -1, "server_created_at", -1));
   BSON_ASSERT (bson_append_array (bson, "missing", -1, array));
   bson_destroy (array);
   BSON_ASSERT_BSON_EQUAL_FILE (bson, "test17.bson");
   bson_destroy (bson);
}


static void
test_bson_append_deep (void)
{
   bson_t *a;
   bson_t *tmp;
   int i;

   a = bson_new ();

   for (i = 0; i < 100; i++) {
      tmp = a;
      a = bson_new ();
      BSON_ASSERT (bson_append_document (a, "a", -1, tmp));
      bson_destroy (tmp);
   }

   BSON_ASSERT_BSON_EQUAL_FILE (a, "test38.bson");

   bson_destroy (a);
}


static void
test_bson_validate_dbref (void)
{
   size_t offset;
   bson_t dbref, child, child2;

   /* should fail, $ref without an $id */
   {
      bson_init (&dbref);
      BSON_APPEND_DOCUMENT_BEGIN (&dbref, "dbref", &child);
      BSON_APPEND_UTF8 (&child, "$ref", "foo");
      bson_append_document_end (&dbref, &child);

      BSON_ASSERT (!bson_validate (&dbref, BSON_VALIDATE_DOLLAR_KEYS, &offset));

      bson_destroy (&dbref);
   }

   /* should fail, $ref with non id field */
   {
      bson_init (&dbref);
      BSON_APPEND_DOCUMENT_BEGIN (&dbref, "dbref", &child);
      BSON_APPEND_UTF8 (&child, "$ref", "foo");
      BSON_APPEND_UTF8 (&child, "extra", "field");
      bson_append_document_end (&dbref, &child);

      BSON_ASSERT (!bson_validate (&dbref, BSON_VALIDATE_DOLLAR_KEYS, &offset));

      bson_destroy (&dbref);
   }

   /* should fail, $ref with $id at the top */
   {
      bson_init (&dbref);
      BSON_APPEND_UTF8 (&dbref, "$ref", "foo");
      BSON_APPEND_UTF8 (&dbref, "$id", "bar");

      BSON_ASSERT (!bson_validate (&dbref, BSON_VALIDATE_DOLLAR_KEYS, &offset));

      bson_destroy (&dbref);
   }

   /* should fail, $ref with $id not first keys */
   {
      bson_init (&dbref);
      BSON_APPEND_DOCUMENT_BEGIN (&dbref, "dbref", &child);
      BSON_APPEND_UTF8 (&child, "extra", "field");
      BSON_APPEND_UTF8 (&child, "$ref", "foo");
      BSON_APPEND_UTF8 (&child, "$id", "bar");
      bson_append_document_end (&dbref, &child);

      BSON_ASSERT (!bson_validate (&dbref, BSON_VALIDATE_DOLLAR_KEYS, &offset));

      bson_destroy (&dbref);
   }

   /* should fail, $ref with $db */
   {
      bson_init (&dbref);
      BSON_APPEND_DOCUMENT_BEGIN (&dbref, "dbref", &child);
      BSON_APPEND_UTF8 (&child, "$ref", "foo");
      BSON_APPEND_UTF8 (&child, "$db", "bar");
      bson_append_document_end (&dbref, &child);

      BSON_ASSERT (!bson_validate (&dbref, BSON_VALIDATE_DOLLAR_KEYS, &offset));

      bson_destroy (&dbref);
   }

   /* should fail, non-string $ref with $id */
   {
      bson_init (&dbref);
      BSON_APPEND_DOCUMENT_BEGIN (&dbref, "dbref", &child);
      BSON_APPEND_INT32 (&child, "$ref", 1);
      BSON_APPEND_UTF8 (&child, "$id", "bar");
      bson_append_document_end (&dbref, &child);

      BSON_ASSERT (!bson_validate (&dbref, BSON_VALIDATE_DOLLAR_KEYS, &offset));

      bson_destroy (&dbref);
   }

   /* should fail, non-string $ref with nothing */
   {
      bson_init (&dbref);
      BSON_APPEND_DOCUMENT_BEGIN (&dbref, "dbref", &child);
      BSON_APPEND_INT32 (&child, "$ref", 1);
      bson_append_document_end (&dbref, &child);

      BSON_ASSERT (!bson_validate (&dbref, BSON_VALIDATE_DOLLAR_KEYS, &offset));

      bson_destroy (&dbref);
   }

   /* should fail, $ref with $id with non-string $db */
   {
      bson_init (&dbref);
      BSON_APPEND_DOCUMENT_BEGIN (&dbref, "dbref", &child);
      BSON_APPEND_UTF8 (&child, "$ref", "foo");
      BSON_APPEND_UTF8 (&child, "$id", "bar");
      BSON_APPEND_INT32 (&child, "$db", 1);
      bson_append_document_end (&dbref, &child);

      BSON_ASSERT (!bson_validate (&dbref, BSON_VALIDATE_DOLLAR_KEYS, &offset));

      bson_destroy (&dbref);
   }

   /* should fail, $ref with $id with non-string $db with stuff after */
   {
      bson_init (&dbref);
      BSON_APPEND_DOCUMENT_BEGIN (&dbref, "dbref", &child);
      BSON_APPEND_UTF8 (&child, "$ref", "foo");
      BSON_APPEND_UTF8 (&child, "$id", "bar");
      BSON_APPEND_INT32 (&child, "$db", 1);
      BSON_APPEND_UTF8 (&child, "extra", "field");
      bson_append_document_end (&dbref, &child);

      BSON_ASSERT (!bson_validate (&dbref, BSON_VALIDATE_DOLLAR_KEYS, &offset));

      bson_destroy (&dbref);
   }

   /* should fail, $ref with $id with stuff, then $db */
   {
      bson_init (&dbref);
      BSON_APPEND_DOCUMENT_BEGIN (&dbref, "dbref", &child);
      BSON_APPEND_UTF8 (&child, "$ref", "foo");
      BSON_APPEND_UTF8 (&child, "$id", "bar");
      BSON_APPEND_UTF8 (&child, "extra", "field");
      BSON_APPEND_UTF8 (&child, "$db", "baz");
      bson_append_document_end (&dbref, &child);

      BSON_ASSERT (!bson_validate (&dbref, BSON_VALIDATE_DOLLAR_KEYS, &offset));

      bson_destroy (&dbref);
   }

   /* should succeed, $ref with $id */
   {
      bson_init (&dbref);
      BSON_APPEND_DOCUMENT_BEGIN (&dbref, "dbref", &child);
      BSON_APPEND_UTF8 (&child, "$ref", "foo");
      BSON_APPEND_UTF8 (&child, "$id", "bar");
      bson_append_document_end (&dbref, &child);

      BSON_ASSERT (bson_validate (&dbref, BSON_VALIDATE_DOLLAR_KEYS, &offset));

      bson_destroy (&dbref);
   }

   /* should succeed, $ref with nested dbref $id */
   {
      bson_init (&dbref);
      BSON_APPEND_DOCUMENT_BEGIN (&dbref, "dbref", &child);
      BSON_APPEND_UTF8 (&child, "$ref", "foo");
      BSON_APPEND_DOCUMENT_BEGIN (&child, "$id", &child2);
      BSON_APPEND_UTF8 (&child2, "$ref", "foo2");
      BSON_APPEND_UTF8 (&child2, "$id", "bar2");
      BSON_APPEND_UTF8 (&child2, "$db", "baz2");
      bson_append_document_end (&child, &child2);
      BSON_APPEND_UTF8 (&child, "$db", "baz");
      bson_append_document_end (&dbref, &child);

      BSON_ASSERT (bson_validate (&dbref, BSON_VALIDATE_DOLLAR_KEYS, &offset));

      bson_destroy (&dbref);
   }

   /* should succeed, $ref with $id and $db */
   {
      bson_init (&dbref);
      BSON_APPEND_DOCUMENT_BEGIN (&dbref, "dbref", &child);
      BSON_APPEND_UTF8 (&child, "$ref", "foo");
      BSON_APPEND_UTF8 (&child, "$id", "bar");
      BSON_APPEND_UTF8 (&child, "$db", "baz");
      bson_append_document_end (&dbref, &child);

      BSON_ASSERT (bson_validate (&dbref, BSON_VALIDATE_DOLLAR_KEYS, &offset));

      bson_destroy (&dbref);
   }

   /* should succeed, $ref with $id and $db and trailing */
   {
      bson_init (&dbref);
      BSON_APPEND_DOCUMENT_BEGIN (&dbref, "dbref", &child);
      BSON_APPEND_UTF8 (&child, "$ref", "foo");
      BSON_APPEND_UTF8 (&child, "$id", "bar");
      BSON_APPEND_UTF8 (&child, "$db", "baz");
      BSON_APPEND_UTF8 (&child, "extra", "field");
      bson_append_document_end (&dbref, &child);

      BSON_ASSERT (bson_validate (&dbref, BSON_VALIDATE_DOLLAR_KEYS, &offset));

      bson_destroy (&dbref);
   }
}


/* BSON spec requires bool value to be exactly 0 or 1 */
static void
test_bson_validate_bool (void)
{
   /* {"b": true}, with implicit NULL at end */
   uint8_t data[] = "\x09\x00\x00\x00\x08\x62\x00\x01";
   bson_t bson;
   bson_iter_t iter;
   size_t err_offset = 0;

   ASSERT (bson_init_static (&bson, data, sizeof data));
   ASSERT (bson_validate (&bson, BSON_VALIDATE_NONE, &err_offset));
   ASSERT (bson_iter_init (&iter, &bson));
   ASSERT (bson_iter_next (&iter));
   ASSERT (BSON_ITER_HOLDS_BOOL (&iter));
   ASSERT (bson_iter_bool (&iter));

   /* replace boolean value 1 with 255 */
   ASSERT (data[7] == '\x01');
   data[7] = (uint8_t) '\xff';

   ASSERT (bson_init_static (&bson, data, 9));
   ASSERT (!bson_validate (&bson, BSON_VALIDATE_NONE, &err_offset));
   BSON_ASSERT_CMPINT (err_offset, ==, 7);

   ASSERT (bson_iter_init (&iter, &bson));
   ASSERT (!bson_iter_next (&iter));
}


/* BSON spec requires the deprecated DBPointer's value to be NULL-termed */
static void
test_bson_validate_dbpointer (void)
{
   /* { "a": DBPointer(ObjectId(...), Collection="b") }, implicit NULL at end */
   uint8_t data[] = "\x1A\x00\x00\x00\x0C\x61\x00\x02\x00\x00\x00\x62\x00"
                    "\x56\xE1\xFC\x72\xE0\xC9\x17\xE9\xC4\x71\x41\x61";

   bson_t bson;
   bson_iter_t iter;
   size_t err_offset = 0;
   uint32_t collection_len;
   const char *collection;
   const bson_oid_t *oid;

   ASSERT (bson_init_static (&bson, data, sizeof data));
   ASSERT (bson_validate (&bson, BSON_VALIDATE_NONE, &err_offset));
   ASSERT (bson_iter_init (&iter, &bson));
   ASSERT (bson_iter_next (&iter));
   ASSERT (BSON_ITER_HOLDS_DBPOINTER (&iter));
   bson_iter_dbpointer (&iter, &collection_len, &collection, &oid);
   BSON_ASSERT_CMPSTR (collection, "b");
   BSON_ASSERT_CMPINT (collection_len, ==, 1);

   /* replace the NULL terminator of "b" with 255 */
   ASSERT (data[12] == '\0');
   data[12] = (uint8_t) '\xff';

   ASSERT (bson_init_static (&bson, data, sizeof data));
   ASSERT (!bson_validate (&bson, BSON_VALIDATE_NONE, &err_offset));
   BSON_ASSERT_CMPINT (err_offset, ==, 12);

   ASSERT (bson_iter_init (&iter, &bson));
   ASSERT (!bson_iter_next (&iter));
}


static void
test_bson_validate (void)
{
   char filename[64];
   size_t offset;
   bson_t *b;
   int i;
   bson_error_t error;

   for (i = 1; i <= 38; i++) {
      bson_snprintf (filename, sizeof filename, "test%u.bson", i);
      b = get_bson (filename);
      BSON_ASSERT (bson_validate (b, BSON_VALIDATE_NONE, &offset));
      bson_destroy (b);
   }

   b = get_bson ("codewscope.bson");
   BSON_ASSERT (bson_validate (b, BSON_VALIDATE_NONE, &offset));
   bson_destroy (b);

   b = get_bson ("empty_key.bson");
   BSON_ASSERT (bson_validate (b,
                               BSON_VALIDATE_NONE | BSON_VALIDATE_UTF8 |
                                  BSON_VALIDATE_DOLLAR_KEYS |
                                  BSON_VALIDATE_DOT_KEYS,
                               &offset));
   bson_destroy (b);

#define VALIDATE_TEST(_filename, _flags, _offset, _flag, _msg)     \
   b = get_bson (_filename);                                       \
   BSON_ASSERT (!bson_validate (b, _flags, &offset));              \
   ASSERT_CMPSIZE_T (offset, ==, (size_t) _offset);                \
   BSON_ASSERT (!bson_validate_with_error (b, _flags, &error));    \
   ASSERT_ERROR_CONTAINS (error, BSON_ERROR_INVALID, _flag, _msg); \
   bson_destroy (b)

   VALIDATE_TEST ("overflow2.bson",
                  BSON_VALIDATE_NONE,
                  9,
                  BSON_VALIDATE_NONE,
                  "corrupt BSON");
   VALIDATE_TEST ("trailingnull.bson",
                  BSON_VALIDATE_NONE,
                  14,
                  BSON_VALIDATE_NONE,
                  "corrupt BSON");
   VALIDATE_TEST ("dollarquery.bson",
                  BSON_VALIDATE_DOLLAR_KEYS | BSON_VALIDATE_DOT_KEYS,
                  4,
                  BSON_VALIDATE_DOLLAR_KEYS,
                  "keys cannot begin with \"$\": \"$query\"");
   VALIDATE_TEST ("dotquery.bson",
                  BSON_VALIDATE_DOLLAR_KEYS | BSON_VALIDATE_DOT_KEYS,
                  4,
                  BSON_VALIDATE_DOT_KEYS,
                  "keys cannot contain \".\": \"abc.def\"");
   VALIDATE_TEST ("overflow3.bson",
                  BSON_VALIDATE_NONE,
                  9,
                  BSON_VALIDATE_NONE,
                  "corrupt BSON");
   /* same outcome as above, despite different flags */
   VALIDATE_TEST ("overflow3.bson",
                  BSON_VALIDATE_UTF8,
                  9,
                  BSON_VALIDATE_NONE,
                  "corrupt BSON");
   VALIDATE_TEST ("overflow4.bson",
                  BSON_VALIDATE_NONE,
                  9,
                  BSON_VALIDATE_NONE,
                  "corrupt BSON");
   VALIDATE_TEST ("empty_key.bson",
                  BSON_VALIDATE_EMPTY_KEYS,
                  4,
                  BSON_VALIDATE_EMPTY_KEYS,
                  "empty key");
   VALIDATE_TEST (
      "test40.bson", BSON_VALIDATE_NONE, 6, BSON_VALIDATE_NONE, "corrupt BSON");
   VALIDATE_TEST (
      "test41.bson", BSON_VALIDATE_NONE, 6, BSON_VALIDATE_NONE, "corrupt BSON");
   VALIDATE_TEST (
      "test42.bson", BSON_VALIDATE_NONE, 6, BSON_VALIDATE_NONE, "corrupt BSON");
   VALIDATE_TEST (
      "test43.bson", BSON_VALIDATE_NONE, 6, BSON_VALIDATE_NONE, "corrupt BSON");
   VALIDATE_TEST ("test44.bson",
                  BSON_VALIDATE_NONE,
                  24,
                  BSON_VALIDATE_NONE,
                  "corrupt BSON");
   VALIDATE_TEST (
      "test45.bson", BSON_VALIDATE_NONE, 6, BSON_VALIDATE_NONE, "corrupt BSON");
   VALIDATE_TEST (
      "test46.bson", BSON_VALIDATE_NONE, 6, BSON_VALIDATE_NONE, "corrupt BSON");
   VALIDATE_TEST (
      "test47.bson", BSON_VALIDATE_NONE, 6, BSON_VALIDATE_NONE, "corrupt BSON");
   VALIDATE_TEST ("test48.bson",
                  BSON_VALIDATE_NONE,
                  14,
                  BSON_VALIDATE_NONE,
                  "corrupt BSON");
   VALIDATE_TEST (
      "test49.bson", BSON_VALIDATE_NONE, 6, BSON_VALIDATE_NONE, "corrupt BSON");
   VALIDATE_TEST ("test50.bson",
                  BSON_VALIDATE_NONE,
                  10,
                  BSON_VALIDATE_NONE,
                  "corrupt code-with-scope");
   VALIDATE_TEST ("test51.bson",
                  BSON_VALIDATE_NONE,
                  10,
                  BSON_VALIDATE_NONE,
                  "corrupt code-with-scope");
   VALIDATE_TEST (
      "test52.bson", BSON_VALIDATE_NONE, 9, BSON_VALIDATE_NONE, "corrupt BSON");
   VALIDATE_TEST (
      "test53.bson", BSON_VALIDATE_NONE, 6, BSON_VALIDATE_NONE, "corrupt BSON");
   VALIDATE_TEST ("test54.bson",
                  BSON_VALIDATE_NONE,
                  12,
                  BSON_VALIDATE_NONE,
                  "corrupt BSON");

   /* DBRef validation */
   b = BCON_NEW ("my_dbref",
                 "{",
                 "$ref",
                 BCON_UTF8 ("collection"),
                 "$id",
                 BCON_INT32 (1),
                 "}");
   BSON_ASSERT (bson_validate_with_error (b, BSON_VALIDATE_NONE, &error));
   BSON_ASSERT (
      bson_validate_with_error (b, BSON_VALIDATE_DOLLAR_KEYS, &error));
   bson_destroy (b);

   /* needs "$ref" before "$id" */
   b = BCON_NEW ("my_dbref", "{", "$id", BCON_INT32 (1), "}");
   BSON_ASSERT (bson_validate_with_error (b, BSON_VALIDATE_NONE, &error));
   BSON_ASSERT (
      !bson_validate_with_error (b, BSON_VALIDATE_DOLLAR_KEYS, &error));
   ASSERT_ERROR_CONTAINS (error,
                          BSON_ERROR_INVALID,
                          BSON_VALIDATE_DOLLAR_KEYS,
                          "keys cannot begin with \"$\": \"$id\"");
   bson_destroy (b);

   /* two $refs */
   b = BCON_NEW ("my_dbref",
                 "{",
                 "$ref",
                 BCON_UTF8 ("collection"),
                 "$ref",
                 BCON_UTF8 ("collection"),
                 "}");
   BSON_ASSERT (bson_validate_with_error (b, BSON_VALIDATE_NONE, &error));
   BSON_ASSERT (
      !bson_validate_with_error (b, BSON_VALIDATE_DOLLAR_KEYS, &error));
   ASSERT_ERROR_CONTAINS (error,
                          BSON_ERROR_INVALID,
                          BSON_VALIDATE_DOLLAR_KEYS,
                          "keys cannot begin with \"$\": \"$ref\"");
   bson_destroy (b);

   /* must not contain invalid key like "extra" */
   b = BCON_NEW ("my_dbref",
                 "{",
                 "$ref",
                 BCON_UTF8 ("collection"),
                 "extra",
                 BCON_INT32 (2),
                 "$id",
                 BCON_INT32 (1),
                 "}");
   BSON_ASSERT (bson_validate_with_error (b, BSON_VALIDATE_NONE, &error));
   BSON_ASSERT (
      !bson_validate_with_error (b, BSON_VALIDATE_DOLLAR_KEYS, &error));
   ASSERT_ERROR_CONTAINS (error,
                          BSON_ERROR_INVALID,
                          BSON_VALIDATE_DOLLAR_KEYS,
                          "invalid key within DBRef subdocument: \"extra\"");
   bson_destroy (b);

#undef VALIDATE_TEST
}


static void
test_bson_init (void)
{
   bson_t b;
   char key[12];
   int i;

   bson_init (&b);
   BSON_ASSERT ((b.flags & BSON_FLAG_INLINE));
   BSON_ASSERT ((b.flags & BSON_FLAG_STATIC));
   BSON_ASSERT (!(b.flags & BSON_FLAG_RDONLY));
   for (i = 0; i < 100; i++) {
      bson_snprintf (key, sizeof key, "%d", i);
      BSON_ASSERT (bson_append_utf8 (&b, key, -1, "bar", -1));
   }
   BSON_ASSERT (!(b.flags & BSON_FLAG_INLINE));
   bson_destroy (&b);
}


static void
test_bson_init_static (void)
{
   static const uint8_t data[5] = {5};
   bson_t b;

   bson_init_static (&b, data, sizeof data);
   BSON_ASSERT ((b.flags & BSON_FLAG_RDONLY));
   bson_destroy (&b);
}


static void
test_bson_new_from_buffer (void)
{
   bson_t *b;
   uint8_t *buf = bson_malloc0 (5);
   size_t len = 5;
   uint32_t len_le = BSON_UINT32_TO_LE (5);

   memcpy (buf, &len_le, sizeof (len_le));

   b = bson_new_from_buffer (&buf, &len, bson_realloc_ctx, NULL);

   BSON_ASSERT (b->flags & BSON_FLAG_NO_FREE);
   BSON_ASSERT (len == 5);
   BSON_ASSERT (b->len == 5);

   bson_append_utf8 (b, "hello", -1, "world", -1);

   BSON_ASSERT (len == 32);
   BSON_ASSERT (b->len == 22);

   bson_destroy (b);

   bson_free (buf);

   buf = NULL;
   len = 0;

   b = bson_new_from_buffer (&buf, &len, bson_realloc_ctx, NULL);

   BSON_ASSERT (b->flags & BSON_FLAG_NO_FREE);
   BSON_ASSERT (len == 5);
   BSON_ASSERT (b->len == 5);

   bson_destroy (b);
   bson_free (buf);
}


static void
test_bson_utf8_key (void)
{
/* euro currency symbol */
#define EU "\xe2\x82\xac"
#define FIVE_EUROS EU EU EU EU EU
   uint32_t length;
   bson_iter_t iter;
   const char *str;
   bson_t *b;
   size_t offset;

   b = get_bson ("eurokey.bson");
   BSON_ASSERT (bson_validate (b, BSON_VALIDATE_NONE, &offset));
   BSON_ASSERT (bson_iter_init (&iter, b));
   BSON_ASSERT (bson_iter_next (&iter));
   BSON_ASSERT (!strcmp (bson_iter_key (&iter), FIVE_EUROS));
   BSON_ASSERT ((str = bson_iter_utf8 (&iter, &length)));
   BSON_ASSERT (length == 15); /* 5 3-byte sequences. */
   BSON_ASSERT (!strcmp (str, FIVE_EUROS));
   bson_destroy (b);
}


static void
test_bson_new_1mm (void)
{
   bson_t *b;
   int i;

   for (i = 0; i < 1000000; i++) {
      b = bson_new ();
      bson_destroy (b);
   }
}


static void
test_bson_init_1mm (void)
{
   bson_t b;
   int i;

   for (i = 0; i < 1000000; i++) {
      bson_init (&b);
      bson_destroy (&b);
   }
}


static void
test_bson_build_child (void)
{
   bson_t b;
   bson_t child;
   bson_t *b2;
   bson_t *child2;

   bson_init (&b);
   BSON_ASSERT (bson_append_document_begin (&b, "foo", -1, &child));
   BSON_ASSERT (bson_append_utf8 (&child, "bar", -1, "baz", -1));
   BSON_ASSERT (bson_append_document_end (&b, &child));

   b2 = bson_new ();
   child2 = bson_new ();
   BSON_ASSERT (bson_append_utf8 (child2, "bar", -1, "baz", -1));
   BSON_ASSERT (bson_append_document (b2, "foo", -1, child2));
   bson_destroy (child2);

   BSON_ASSERT (b.len == b2->len);
   BSON_ASSERT_BSON_EQUAL (&b, b2);

   bson_destroy (&b);
   bson_destroy (b2);
}


static void
test_bson_build_child_array (void)
{
   bson_t b;
   bson_t child;
   bson_t *b2;
   bson_t *child2;

   bson_init (&b);
   BSON_ASSERT (bson_append_array_begin (&b, "foo", -1, &child));
   BSON_ASSERT (bson_append_utf8 (&child, "0", -1, "baz", -1));
   BSON_ASSERT (bson_append_array_end (&b, &child));

   b2 = bson_new ();
   child2 = bson_new ();
   BSON_ASSERT (bson_append_utf8 (child2, "0", -1, "baz", -1));
   BSON_ASSERT (bson_append_array (b2, "foo", -1, child2));
   bson_destroy (child2);

   BSON_ASSERT (b.len == b2->len);
   BSON_ASSERT_BSON_EQUAL (&b, b2);

   bson_destroy (&b);
   bson_destroy (b2);
}


static void
test_bson_build_child_deep_1 (bson_t *b, int *count)
{
   bson_t child;

   (*count)++;

   BSON_ASSERT (bson_append_document_begin (b, "b", -1, &child));
   BSON_ASSERT (!(b->flags & BSON_FLAG_INLINE));
   BSON_ASSERT ((b->flags & BSON_FLAG_IN_CHILD));
   BSON_ASSERT (!(child.flags & BSON_FLAG_INLINE));
   BSON_ASSERT ((child.flags & BSON_FLAG_STATIC));
   BSON_ASSERT ((child.flags & BSON_FLAG_CHILD));

   if (*count < 100) {
      test_bson_build_child_deep_1 (&child, count);
   } else {
      BSON_ASSERT (bson_append_int32 (&child, "b", -1, 1234));
   }

   BSON_ASSERT (bson_append_document_end (b, &child));
   BSON_ASSERT (!(b->flags & BSON_FLAG_IN_CHILD));
}


static void
test_bson_build_child_deep (void)
{
   union {
      bson_t b;
      bson_impl_alloc_t a;
   } u;
   int count = 0;

   bson_init (&u.b);
   BSON_ASSERT ((u.b.flags & BSON_FLAG_INLINE));
   test_bson_build_child_deep_1 (&u.b, &count);
   BSON_ASSERT (!(u.b.flags & BSON_FLAG_INLINE));
   BSON_ASSERT ((u.b.flags & BSON_FLAG_STATIC));
   BSON_ASSERT (!(u.b.flags & BSON_FLAG_NO_FREE));
   BSON_ASSERT (!(u.b.flags & BSON_FLAG_RDONLY));
   BSON_ASSERT (bson_validate (&u.b, BSON_VALIDATE_NONE, NULL));
   BSON_ASSERT (((bson_impl_alloc_t *) &u.b)->alloclen == 1024);
   BSON_ASSERT_BSON_EQUAL_FILE (&u.b, "test39.bson");
   bson_destroy (&u.b);
}


static void
test_bson_build_child_deep_no_begin_end_1 (bson_t *b, int *count)
{
   bson_t child;

   (*count)++;

   bson_init (&child);
   if (*count < 100) {
      test_bson_build_child_deep_1 (&child, count);
   } else {
      BSON_ASSERT (bson_append_int32 (&child, "b", -1, 1234));
   }
   BSON_ASSERT (bson_append_document (b, "b", -1, &child));
   bson_destroy (&child);
}


static void
test_bson_build_child_deep_no_begin_end (void)
{
   union {
      bson_t b;
      bson_impl_alloc_t a;
   } u;

   int count = 0;

   bson_init (&u.b);
   test_bson_build_child_deep_no_begin_end_1 (&u.b, &count);
   BSON_ASSERT (bson_validate (&u.b, BSON_VALIDATE_NONE, NULL));
   BSON_ASSERT (u.a.alloclen == 1024);
   BSON_ASSERT_BSON_EQUAL_FILE (&u.b, "test39.bson");
   bson_destroy (&u.b);
}


static void
test_bson_count_keys (void)
{
   bson_t b;

   bson_init (&b);
   BSON_ASSERT (bson_append_int32 (&b, "0", -1, 0));
   BSON_ASSERT (bson_append_int32 (&b, "1", -1, 1));
   BSON_ASSERT (bson_append_int32 (&b, "2", -1, 2));
   BSON_ASSERT_CMPINT (bson_count_keys (&b), ==, 3);
   bson_destroy (&b);
}


static void
test_bson_copy (void)
{
   bson_t b;
   bson_t *c;

   bson_init (&b);
   BSON_ASSERT (bson_append_int32 (&b, "foobar", -1, 1234));
   c = bson_copy (&b);
   BSON_ASSERT_BSON_EQUAL (&b, c);
   bson_destroy (c);
   bson_destroy (&b);
}


static void
test_bson_copy_to (void)
{
   bson_t b;
   bson_t c;
   int i;

   /*
    * Test inline structure copy.
    */
   bson_init (&b);
   BSON_ASSERT (bson_append_int32 (&b, "foobar", -1, 1234));
   bson_copy_to (&b, &c);
   BSON_ASSERT_BSON_EQUAL (&b, &c);
   bson_destroy (&c);
   bson_destroy (&b);

   /*
    * Test malloced copy.
    */
   bson_init (&b);
   for (i = 0; i < 1000; i++) {
      BSON_ASSERT (bson_append_int32 (&b, "foobar", -1, 1234));
   }
   bson_copy_to (&b, &c);
   BSON_ASSERT_BSON_EQUAL (&b, &c);
   bson_destroy (&c);
   bson_destroy (&b);
}


static void
test_bson_copy_to_excluding_noinit (void)
{
   bson_iter_t iter;
   bool r;
   bson_t b;
   bson_t c;
   int i;

   bson_init (&b);
   bson_append_int32 (&b, "a", 1, 1);
   bson_append_int32 (&b, "b", 1, 2);

   bson_init (&c);
   bson_copy_to_excluding_noinit (&b, &c, "b", NULL);
   r = bson_iter_init_find (&iter, &c, "a");
   BSON_ASSERT (r);
   r = bson_iter_init_find (&iter, &c, "b");
   BSON_ASSERT (!r);

   i = bson_count_keys (&b);
   BSON_ASSERT_CMPINT (i, ==, 2);

   i = bson_count_keys (&c);
   BSON_ASSERT_CMPINT (i, ==, 1);

   bson_destroy (&b);
   bson_destroy (&c);
}


static void
test_bson_append_overflow (void)
{
   const char *key = "a";
   uint32_t len;
   bson_t b;

   len = BSON_MAX_SIZE;
   len -= 4; /* len */
   len -= 1; /* type */
   len -= 1; /* value */
   len -= 1; /* end byte */

   bson_init (&b);
   BSON_ASSERT (!bson_append_bool (&b, key, len, true));
   bson_destroy (&b);
}


static void
test_bson_initializer (void)
{
   bson_t b = BSON_INITIALIZER;

   BSON_ASSERT (bson_empty (&b));
   bson_append_bool (&b, "foo", -1, true);
   BSON_ASSERT (!bson_empty (&b));
   bson_destroy (&b);
}


static void
test_bson_concat (void)
{
   bson_t a = BSON_INITIALIZER;
   bson_t b = BSON_INITIALIZER;
   bson_t c = BSON_INITIALIZER;

   bson_append_int32 (&a, "abc", 3, 1);
   bson_append_int32 (&b, "def", 3, 1);
   bson_concat (&a, &b);

   bson_append_int32 (&c, "abc", 3, 1);
   bson_append_int32 (&c, "def", 3, 1);

   BSON_ASSERT (0 == bson_compare (&c, &a));

   bson_destroy (&a);
   bson_destroy (&b);
   bson_destroy (&c);
}


static void
test_bson_reinit (void)
{
   bson_t b = BSON_INITIALIZER;
   int i;

   for (i = 0; i < 1000; i++) {
      bson_append_int32 (&b, "", 0, i);
   }

   bson_reinit (&b);

   for (i = 0; i < 1000; i++) {
      bson_append_int32 (&b, "", 0, i);
   }

   bson_destroy (&b);
}


static void
test_bson_macros (void)
{
   const uint8_t data[] = {1, 2, 3, 4};
   bson_t b = BSON_INITIALIZER;
   bson_t ar = BSON_INITIALIZER;
   bson_decimal128_t dec;
   bson_oid_t oid;
   struct timeval tv;
   time_t t;

   dec.high = 0x3040000000000000ULL;
   dec.low = 0x0ULL;

   t = time (NULL);
#ifdef BSON_OS_WIN32
   tv.tv_sec = (long) t;
#else
   tv.tv_sec = t;
#endif
   tv.tv_usec = 0;

   bson_oid_init (&oid, NULL);

   BSON_APPEND_ARRAY (&b, "0", &ar);
   BSON_APPEND_BINARY (&b, "1", 0, data, sizeof data);
   BSON_APPEND_BOOL (&b, "2", true);
   BSON_APPEND_CODE (&b, "3", "function(){}");
   BSON_APPEND_CODE_WITH_SCOPE (&b, "4", "function(){}", &ar);
   BSON_APPEND_DOUBLE (&b, "6", 123.45);
   BSON_APPEND_DOCUMENT (&b, "7", &ar);
   BSON_APPEND_INT32 (&b, "8", 123);
   BSON_APPEND_INT64 (&b, "9", 456);
   BSON_APPEND_MINKEY (&b, "10");
   BSON_APPEND_MAXKEY (&b, "11");
   BSON_APPEND_NULL (&b, "12");
   BSON_APPEND_OID (&b, "13", &oid);
   BSON_APPEND_REGEX (&b, "14", "^abc", "i");
   BSON_APPEND_UTF8 (&b, "15", "utf8");
   BSON_APPEND_SYMBOL (&b, "16", "symbol");
   BSON_APPEND_TIME_T (&b, "17", t);
   BSON_APPEND_TIMEVAL (&b, "18", &tv);
   BSON_APPEND_DATE_TIME (&b, "19", 123);
   BSON_APPEND_TIMESTAMP (&b, "20", 123, 0);
   BSON_APPEND_UNDEFINED (&b, "21");
   BSON_APPEND_DECIMAL128 (&b, "22", &dec);

   bson_destroy (&b);
   bson_destroy (&ar);
}


static void
test_bson_clear (void)
{
   bson_t *doc = NULL;

   bson_clear (&doc);
   BSON_ASSERT (doc == NULL);

   doc = bson_new ();
   BSON_ASSERT (doc != NULL);
   bson_clear (&doc);
   BSON_ASSERT (doc == NULL);
}


static void
bloat (bson_t *b)
{
   uint32_t i;
   char buf[16];
   const char *key;

   for (i = 0; i < 100; i++) {
      bson_uint32_to_string (i, &key, buf, sizeof buf);
      BSON_APPEND_UTF8 (b, key, "long useless value foo bar baz quux quizzle");
   }

   /* spilled over */
   ASSERT (!(b->flags & BSON_FLAG_INLINE));
}


static void
test_bson_steal (void)
{
   bson_t stack_alloced;
   bson_t *heap_alloced;
   bson_t dst;
   uint8_t *alloc;
   uint8_t *buf;
   size_t len;
   uint32_t len_le;

   /* inline, stack-allocated */
   bson_init (&stack_alloced);
   BSON_APPEND_INT32 (&stack_alloced, "a", 1);
   ASSERT (bson_steal (&dst, &stack_alloced));
   ASSERT (bson_has_field (&dst, "a"));
   ASSERT (dst.flags & BSON_FLAG_INLINE);
   /* src was invalidated */
   ASSERT (!bson_validate (&stack_alloced, BSON_VALIDATE_NONE, 0));
   bson_destroy (&dst);

   /* spilled over, stack-allocated */
   bson_init (&stack_alloced);
   bloat (&stack_alloced);
   alloc = ((bson_impl_alloc_t *) &stack_alloced)->alloc;
   ASSERT (bson_steal (&dst, &stack_alloced));
   /* data was transferred */
   ASSERT (alloc == ((bson_impl_alloc_t *) &dst)->alloc);
   ASSERT (bson_has_field (&dst, "99"));
   ASSERT (!(dst.flags & BSON_FLAG_INLINE));
   ASSERT (!bson_validate (&stack_alloced, BSON_VALIDATE_NONE, 0));
   bson_destroy (&dst);

   /* inline, heap-allocated */
   heap_alloced = bson_new ();
   BSON_APPEND_INT32 (heap_alloced, "a", 1);
   ASSERT (bson_steal (&dst, heap_alloced));
   ASSERT (bson_has_field (&dst, "a"));
   ASSERT (dst.flags & BSON_FLAG_INLINE);
   bson_destroy (&dst);

   /* spilled over, heap-allocated */
   heap_alloced = bson_new ();
   bloat (heap_alloced);
   alloc = ((bson_impl_alloc_t *) heap_alloced)->alloc;
   ASSERT (bson_steal (&dst, heap_alloced));
   /* data was transferred */
   ASSERT (alloc == ((bson_impl_alloc_t *) &dst)->alloc);
   ASSERT (bson_has_field (&dst, "99"));
   ASSERT (!(dst.flags & BSON_FLAG_INLINE));
   bson_destroy (&dst);

   /* test stealing from a bson created with bson_new_from_buffer */
   buf = bson_malloc0 (5);
   len = 5;
   len_le = BSON_UINT32_TO_LE (5);
   memcpy (buf, &len_le, sizeof (len_le));
   heap_alloced = bson_new_from_buffer (&buf, &len, bson_realloc_ctx, NULL);
   ASSERT (bson_steal (&dst, heap_alloced));
   ASSERT (dst.flags & BSON_FLAG_NO_FREE);
   ASSERT (dst.flags & BSON_FLAG_STATIC);
   ASSERT (((bson_impl_alloc_t *) &dst)->realloc == bson_realloc_ctx);
   ASSERT (((bson_impl_alloc_t *) &dst)->realloc_func_ctx == NULL);
   bson_destroy (&dst);
   bson_free (buf);
}

static void
BSON_ASSERT_KEY_AND_VALUE (const bson_t *bson)
{
   bson_iter_t iter;

   ASSERT (bson_iter_init_find (&iter, bson, "key"));
   ASSERT (BSON_ITER_HOLDS_UTF8 (&iter));
   ASSERT_CMPSTR ("value", bson_iter_utf8 (&iter, NULL));
}


static void
test_bson_reserve_buffer (void)
{
   bson_t src = BSON_INITIALIZER;
   bson_t stack_alloced;
   bson_t *heap_alloced;
   uint8_t *buf;

   /* inline, stack-allocated */
   bson_init (&stack_alloced);
   BSON_APPEND_UTF8 (&src, "key", "value");
   ASSERT ((buf = bson_reserve_buffer (&stack_alloced, src.len)));
   ASSERT_CMPUINT32 (src.len, ==, stack_alloced.len);
   ASSERT (stack_alloced.flags & BSON_FLAG_INLINE);
   memcpy (buf, ((bson_impl_inline_t *) &src)->data, src.len);
   /* data was transferred */
   BSON_ASSERT_KEY_AND_VALUE (&stack_alloced);
   bson_destroy (&stack_alloced);

   /* spilled over, stack-allocated */
   bloat (&src);
   bson_init (&stack_alloced);
   ASSERT ((buf = bson_reserve_buffer (&stack_alloced, src.len)));
   ASSERT_CMPUINT32 (src.len, ==, stack_alloced.len);
   ASSERT (!(stack_alloced.flags & BSON_FLAG_INLINE));
   memcpy (buf, ((bson_impl_alloc_t *) &src)->alloc, src.len);
   BSON_ASSERT_KEY_AND_VALUE (&stack_alloced);
   ASSERT (bson_has_field (&stack_alloced, "99"));
   bson_destroy (&src);
   bson_destroy (&stack_alloced);

   /* inline, heap-allocated */
   heap_alloced = bson_new ();
   bson_init (&src);
   BSON_APPEND_UTF8 (&src, "key", "value");
   ASSERT ((buf = bson_reserve_buffer (heap_alloced, src.len)));
   ASSERT_CMPUINT32 (src.len, ==, heap_alloced->len);
   ASSERT (heap_alloced->flags & BSON_FLAG_INLINE);
   memcpy (buf, ((bson_impl_inline_t *) &src)->data, src.len);
   BSON_ASSERT_KEY_AND_VALUE (heap_alloced);
   bson_destroy (heap_alloced);

   /* spilled over, heap-allocated */
   heap_alloced = bson_new ();
   bloat (&src);
   ASSERT ((buf = bson_reserve_buffer (heap_alloced, src.len)));
   ASSERT_CMPUINT32 (src.len, ==, heap_alloced->len);
   ASSERT (!(heap_alloced->flags & BSON_FLAG_INLINE));
   memcpy (buf, ((bson_impl_alloc_t *) &src)->alloc, src.len);
   BSON_ASSERT_KEY_AND_VALUE (heap_alloced);
   ASSERT (bson_has_field (heap_alloced, "99"));

   bson_destroy (&src);
   bson_destroy (heap_alloced);
}


static void
test_bson_reserve_buffer_errors (void)
{
   bson_t bson = BSON_INITIALIZER;
   bson_t child;
   uint8_t data[5] = {0};
   uint32_t len_le;

   /* too big */
   ASSERT (!bson_reserve_buffer (&bson, (uint32_t) (INT32_MAX - bson.len - 1)));

   /* make a static bson, it refuses bson_reserve_buffer since it's read-only */
   len_le = BSON_UINT32_TO_LE (5);
   memcpy (data, &len_le, sizeof (len_le));
   ASSERT (bson_init_static (&bson, data, sizeof data));
   ASSERT (!bson_reserve_buffer (&bson, 10));

   /* parent's and child's buffers are locked */
   bson_init (&bson);
   BSON_APPEND_DOCUMENT_BEGIN (&bson, "child", &child);
   ASSERT (!bson_reserve_buffer (&bson, 10));
   ASSERT (!bson_reserve_buffer (&child, 10));
   /* unlock parent's buffer */
   bson_append_document_end (&bson, &child);
   ASSERT (bson_reserve_buffer (&bson, 10));

   bson_destroy (&bson);
}


static void
test_bson_destroy_with_steal (void)
{
   bson_t *b1;
   bson_t b2;
   uint32_t len = 0;
   uint8_t *data;
   int i;

   b1 = bson_new ();
   for (i = 0; i < 100; i++) {
      BSON_APPEND_INT32 (b1, "some-key", i);
   }

   data = bson_destroy_with_steal (b1, true, &len);
   BSON_ASSERT (data);
   BSON_ASSERT (len == 1405);
   bson_free (data);
   data = NULL;

   bson_init (&b2);
   len = 0;
   for (i = 0; i < 100; i++) {
      BSON_APPEND_INT32 (&b2, "some-key", i);
   }
   BSON_ASSERT (!bson_destroy_with_steal (&b2, false, &len));
   BSON_ASSERT (len == 1405);

   bson_init (&b2);
   BSON_ASSERT (!bson_destroy_with_steal (&b2, false, NULL));

   bson_init (&b2);
   data = bson_destroy_with_steal (&b2, true, &len);
   BSON_ASSERT (data);
   BSON_ASSERT (len == 5);
   bson_free (data);
   data = NULL;
}


static void
test_bson_has_field (void)
{
   bson_t *b;
   bool r;

   b = BCON_NEW ("foo", "[", "{", "bar", BCON_INT32 (1), "}", "]");

   r = bson_has_field (b, "foo");
   BSON_ASSERT (r);

   r = bson_has_field (b, "foo.0");
   BSON_ASSERT (r);

   r = bson_has_field (b, "foo.0.bar");
   BSON_ASSERT (r);

   r = bson_has_field (b, "0");
   BSON_ASSERT (!r);

   r = bson_has_field (b, "bar");
   BSON_ASSERT (!r);

   r = bson_has_field (b, "0.bar");
   BSON_ASSERT (!r);

   bson_destroy (b);
}


static void
test_next_power_of_two (void)
{
   size_t s;

   s = 3;
   s = bson_next_power_of_two (s);
   BSON_ASSERT (s == 4);

   s = 4;
   s = bson_next_power_of_two (s);
   BSON_ASSERT (s == 4);

   s = 33;
   s = bson_next_power_of_two (s);
   BSON_ASSERT (s == 64);

   s = 91;
   s = bson_next_power_of_two (s);
   BSON_ASSERT (s == 128);

   s = 939524096UL;
   s = bson_next_power_of_two (s);
   BSON_ASSERT (s == 1073741824);

   s = 1073741824UL;
   s = bson_next_power_of_two (s);
   BSON_ASSERT (s == 1073741824UL);

#if BSON_WORD_SIZE == 64
   s = 4294967296LL;
   s = bson_next_power_of_two (s);
   BSON_ASSERT (s == 4294967296LL);

   s = 4294967297LL;
   s = bson_next_power_of_two (s);
   BSON_ASSERT (s == 8589934592LL);

   s = 17179901952LL;
   s = bson_next_power_of_two (s);
   BSON_ASSERT (s == 34359738368LL);

   s = 9223372036854775807ULL;
   s = bson_next_power_of_two (s);
   BSON_ASSERT (s == 9223372036854775808ULL);

   s = 36028795806651656ULL;
   s = bson_next_power_of_two (s);
   BSON_ASSERT (s == 36028797018963968ULL);
#endif
}


void
visit_corrupt (const bson_iter_t *iter, void *data)
{
   *((bool *) data) = true;
}


static void
test_bson_visit_invalid_field (void)
{
   /* key is invalid utf-8 char: {"\x80": 1} */
   const char data[] = "\x0c\x00\x00\x00\x10\x80\x00\x01\x00\x00\x00\x00";
   bson_t b;
   bson_iter_t iter;
   bson_visitor_t visitor = {0};
   bool visited = false;

   visitor.visit_corrupt = visit_corrupt;
   BSON_ASSERT (bson_init_static (&b, (const uint8_t *) data, sizeof data - 1));
   BSON_ASSERT (bson_iter_init (&iter, &b));
   BSON_ASSERT (!bson_iter_visit_all (&iter, &visitor, (void *) &visited));
   BSON_ASSERT (visited);
}


typedef struct {
   bool visited;
   const char *key;
   uint32_t type_code;
} unsupported_type_test_data_t;


void
visit_unsupported_type (const bson_iter_t *iter,
                        const char *key,
                        uint32_t type_code,
                        void *data)
{
   unsupported_type_test_data_t *context;

   context = (unsupported_type_test_data_t *) data;
   context->visited = true;
   context->key = key;
   context->type_code = type_code;
}


static void
test_bson_visit_unsupported_type (void)
{
   /* {k: 1}, but instead of BSON type 0x10 (int32), use unknown type 0x33 */
   const char data[] = "\x0c\x00\x00\x00\x33k\x00\x01\x00\x00\x00\x00";
   bson_t b;
   bson_iter_t iter;
   unsupported_type_test_data_t context = {0};
   bson_visitor_t visitor = {0};

   visitor.visit_unsupported_type = visit_unsupported_type;

   BSON_ASSERT (bson_init_static (&b, (const uint8_t *) data, sizeof data - 1));
   BSON_ASSERT (bson_iter_init (&iter, &b));
   bson_iter_visit_all (&iter, &visitor, (void *) &context);
   BSON_ASSERT (!bson_iter_next (&iter));
   BSON_ASSERT (context.visited);
   BSON_ASSERT (!strcmp (context.key, "k"));
   BSON_ASSERT (context.type_code == '\x33');
}


static void
test_bson_visit_unsupported_type_bad_key (void)
{
   /* key is invalid utf-8 char, '\x80' */
   const char data[] = "\x0c\x00\x00\x00\x33\x80\x00\x01\x00\x00\x00\x00";
   bson_t b;
   bson_iter_t iter;
   unsupported_type_test_data_t context = {0};
   bson_visitor_t visitor = {0};

   visitor.visit_unsupported_type = visit_unsupported_type;

   BSON_ASSERT (bson_init_static (&b, (const uint8_t *) data, sizeof data - 1));
   BSON_ASSERT (bson_iter_init (&iter, &b));
   bson_iter_visit_all (&iter, &visitor, (void *) &context);
   BSON_ASSERT (!bson_iter_next (&iter));

   /* unsupported type error wasn't reported, because the bson is corrupt */
   BSON_ASSERT (!context.visited);
}


static void
test_bson_visit_unsupported_type_empty_key (void)
{
   /* {"": 1}, but instead of BSON type 0x10 (int32), use unknown type 0x33 */
   const char data[] = "\x0b\x00\x00\x00\x33\x00\x01\x00\x00\x00\x00";
   bson_t b;
   bson_iter_t iter;
   unsupported_type_test_data_t context = {0};
   bson_visitor_t visitor = {0};

   visitor.visit_unsupported_type = visit_unsupported_type;

   BSON_ASSERT (bson_init_static (&b, (const uint8_t *) data, sizeof data - 1));
   BSON_ASSERT (bson_iter_init (&iter, &b));
   bson_iter_visit_all (&iter, &visitor, (void *) &context);
   BSON_ASSERT (!bson_iter_next (&iter));
   BSON_ASSERT (context.visited);
   BSON_ASSERT (!strcmp (context.key, ""));
   BSON_ASSERT (context.type_code == '\x33');
}


static void
test_bson_subtype_2 (void)
{
   bson_t b;
   /* taken from BSON Corpus Tests */
   const char ok[] = "\x13\x00\x00\x00\x05\x78\x00\x06\x00\x00\x00\x02\x02\x00"
                     "\x00\x00\xff\xff\x00";

   /* Deprecated subtype 0x02 includes a redundant length inside the binary
    * payload. Check that we validate this length.
    */
   const char len_too_long[] = "\x13\x00\x00\x00\x05\x78\x00\x06\x00\x00\x00"
                               "\x02\x03\x00\x00\x00\xFF\xFF\x00";
   const char len_too_short[] = "\x13\x00\x00\x00\x05\x78\x00\x06\x00\x00\x00"
                                "\x02\x01\x00\x00\x00\xFF\xFF\x00";
   const char len_negative[] = "\x13\x00\x00\x00\x05\x78\x00\x06\x00\x00\x00"
                               "\x02\xFF\xFF\xFF\xFF\xFF\xFF\x00";

   bson_t *bson_ok = BCON_NEW ("x", BCON_BIN (2, (uint8_t *) "\xff\xff", 2));

   BSON_ASSERT (bson_init_static (&b, (uint8_t *) ok, sizeof (ok) - 1));
   BSON_ASSERT (bson_validate (&b, BSON_VALIDATE_NONE, 0));
   BSON_ASSERT (0 == bson_compare (&b, bson_ok));

   BSON_ASSERT (bson_init_static (
      &b, (uint8_t *) len_too_long, sizeof (len_too_long) - 1));
   BSON_ASSERT (!bson_validate (&b, BSON_VALIDATE_NONE, 0));

   BSON_ASSERT (bson_init_static (
      &b, (uint8_t *) len_too_short, sizeof (len_too_short) - 1));
   BSON_ASSERT (!bson_validate (&b, BSON_VALIDATE_NONE, 0));

   BSON_ASSERT (bson_init_static (
      &b, (uint8_t *) len_negative, sizeof (len_negative) - 1));
   BSON_ASSERT (!bson_validate (&b, BSON_VALIDATE_NONE, 0));

   bson_destroy (bson_ok);
}


void
test_bson_install (TestSuite *suite)
{
   TestSuite_Add (suite, "/bson/new", test_bson_new);
   TestSuite_Add (suite, "/bson/new_from_buffer", test_bson_new_from_buffer);
   TestSuite_Add (suite, "/bson/init", test_bson_init);
   TestSuite_Add (suite, "/bson/init_static", test_bson_init_static);
   TestSuite_Add (suite, "/bson/basic", test_bson_alloc);
   TestSuite_Add (suite, "/bson/append_overflow", test_bson_append_overflow);
   TestSuite_Add (suite, "/bson/append_array", test_bson_append_array);
   TestSuite_Add (suite, "/bson/append_binary", test_bson_append_binary);
   TestSuite_Add (suite,
                  "/bson/append_binary_deprecated",
                  test_bson_append_binary_deprecated);
   TestSuite_Add (suite, "/bson/append_bool", test_bson_append_bool);
   TestSuite_Add (suite, "/bson/append_code", test_bson_append_code);
   TestSuite_Add (
      suite, "/bson/append_code_with_scope", test_bson_append_code_with_scope);
   TestSuite_Add (suite, "/bson/append_dbpointer", test_bson_append_dbpointer);
   TestSuite_Add (suite, "/bson/append_document", test_bson_append_document);
   TestSuite_Add (suite, "/bson/append_double", test_bson_append_double);
   TestSuite_Add (suite, "/bson/append_int32", test_bson_append_int32);
   TestSuite_Add (suite, "/bson/append_int64", test_bson_append_int64);
   TestSuite_Add (
      suite, "/bson/append_decimal128", test_bson_append_decimal128);
   TestSuite_Add (suite, "/bson/append_iter", test_bson_append_iter);
   TestSuite_Add (suite, "/bson/append_maxkey", test_bson_append_maxkey);
   TestSuite_Add (suite, "/bson/append_minkey", test_bson_append_minkey);
   TestSuite_Add (suite, "/bson/append_null", test_bson_append_null);
   TestSuite_Add (suite, "/bson/append_oid", test_bson_append_oid);
   TestSuite_Add (suite, "/bson/append_regex", test_bson_append_regex);
   TestSuite_Add (suite, "/bson/append_utf8", test_bson_append_utf8);
   TestSuite_Add (suite, "/bson/append_symbol", test_bson_append_symbol);
   TestSuite_Add (suite, "/bson/append_time_t", test_bson_append_time_t);
   TestSuite_Add (suite, "/bson/append_timestamp", test_bson_append_timestamp);
   TestSuite_Add (suite, "/bson/append_timeval", test_bson_append_timeval);
   TestSuite_Add (suite, "/bson/append_undefined", test_bson_append_undefined);
   TestSuite_Add (suite, "/bson/append_general", test_bson_append_general);
   TestSuite_Add (suite, "/bson/append_deep", test_bson_append_deep);
   TestSuite_Add (suite, "/bson/utf8_key", test_bson_utf8_key);
   TestSuite_Add (suite, "/bson/validate", test_bson_validate);
   TestSuite_Add (suite, "/bson/validate/dbref", test_bson_validate_dbref);
   TestSuite_Add (suite, "/bson/validate/bool", test_bson_validate_bool);
   TestSuite_Add (
      suite, "/bson/validate/dbpointer", test_bson_validate_dbpointer);
   TestSuite_Add (suite, "/bson/new_1mm", test_bson_new_1mm);
   TestSuite_Add (suite, "/bson/init_1mm", test_bson_init_1mm);
   TestSuite_Add (suite, "/bson/build_child", test_bson_build_child);
   TestSuite_Add (suite, "/bson/build_child_deep", test_bson_build_child_deep);
   TestSuite_Add (suite,
                  "/bson/build_child_deep_no_begin_end",
                  test_bson_build_child_deep_no_begin_end);
   TestSuite_Add (
      suite, "/bson/build_child_array", test_bson_build_child_array);
   TestSuite_Add (suite, "/bson/count", test_bson_count_keys);
   TestSuite_Add (suite, "/bson/copy", test_bson_copy);
   TestSuite_Add (suite, "/bson/copy_to", test_bson_copy_to);
   TestSuite_Add (suite,
                  "/bson/copy_to_excluding_noinit",
                  test_bson_copy_to_excluding_noinit);
   TestSuite_Add (suite, "/bson/initializer", test_bson_initializer);
   TestSuite_Add (suite, "/bson/concat", test_bson_concat);
   TestSuite_Add (suite, "/bson/reinit", test_bson_reinit);
   TestSuite_Add (suite, "/bson/macros", test_bson_macros);
   TestSuite_Add (suite, "/bson/clear", test_bson_clear);
   TestSuite_Add (suite, "/bson/steal", test_bson_steal);
   TestSuite_Add (suite, "/bson/reserve_buffer", test_bson_reserve_buffer);
   TestSuite_Add (
      suite, "/bson/reserve_buffer/errors", test_bson_reserve_buffer_errors);
   TestSuite_Add (
      suite, "/bson/destroy_with_steal", test_bson_destroy_with_steal);
   TestSuite_Add (suite, "/bson/has_field", test_bson_has_field);
   TestSuite_Add (
      suite, "/bson/visit_invalid_field", test_bson_visit_invalid_field);
   TestSuite_Add (
      suite, "/bson/unsupported_type", test_bson_visit_unsupported_type);
   TestSuite_Add (suite,
                  "/bson/unsupported_type/bad_key",
                  test_bson_visit_unsupported_type_bad_key);
   TestSuite_Add (suite,
                  "/bson/unsupported_type/empty_key",
                  test_bson_visit_unsupported_type_empty_key);
   TestSuite_Add (suite, "/bson/binary_subtype_2", test_bson_subtype_2);
   TestSuite_Add (suite, "/util/next_power_of_two", test_next_power_of_two);
}
