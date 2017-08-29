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
#include <fcntl.h>
#include <time.h>

#include "bson-tests.h"
#include "TestSuite.h"

#ifndef BINARY_DIR
#define BINARY_DIR "tests/binary"
#endif


#define FUZZ_N_PASSES 100000

static bson_t *
get_bson (const char *filename)
{
   uint8_t buf[4096];
   bson_t *b;
   ssize_t len;
   int fd;

   if (-1 == (fd = bson_open (filename, O_RDONLY))) {
      fprintf (stderr, "Failed to open: %s\n", filename);
      abort ();
   }
   if ((len = bson_read (fd, buf, sizeof buf)) < 0) {
      fprintf (stderr, "Failed to read: %s\n", filename);
      abort ();
   }
   BSON_ASSERT (len > 0);
   b = bson_new_from_data (buf, (uint32_t) len);
   bson_close (fd);

   return b;
}


static void
test_bson_iter_utf8 (void)
{
   uint32_t len = 0;
   bson_iter_t iter;
   bson_t *b;
   char *s;

   b = bson_new ();
   BSON_ASSERT (bson_append_utf8 (b, "foo", -1, "bar", -1));
   BSON_ASSERT (bson_append_utf8 (b, "bar", -1, "baz", -1));
   BSON_ASSERT (bson_iter_init (&iter, b));
   BSON_ASSERT (bson_iter_next (&iter));
   BSON_ASSERT (BSON_ITER_HOLDS_UTF8 (&iter));
   BSON_ASSERT (!strcmp (bson_iter_key (&iter), "foo"));
   BSON_ASSERT (!strcmp (bson_iter_utf8 (&iter, NULL), "bar"));
   s = bson_iter_dup_utf8 (&iter, &len);
   BSON_ASSERT_CMPSTR ("bar", s);
   BSON_ASSERT_CMPINT (len, ==, 3);
   bson_free (s);
   BSON_ASSERT (bson_iter_next (&iter));
   BSON_ASSERT (BSON_ITER_HOLDS_UTF8 (&iter));
   BSON_ASSERT (!strcmp (bson_iter_key (&iter), "bar"));
   BSON_ASSERT (!strcmp (bson_iter_utf8 (&iter, NULL), "baz"));
   BSON_ASSERT (!bson_iter_next (&iter));
   bson_destroy (b);
}


static void
test_bson_iter_mixed (void)
{
   bson_iter_t iter;
   bson_decimal128_t iter_value;
   bson_decimal128_t value;
   bson_t *b;
   bson_t *b2;

   b = bson_new ();
   b2 = bson_new ();

   value.high = 0;
   value.low = 1;

   BSON_ASSERT (bson_append_utf8 (b2, "foo", -1, "bar", -1));
   BSON_ASSERT (bson_append_code (b, "0", -1, "var a = {};"));
   BSON_ASSERT (bson_append_code_with_scope (b, "1", -1, "var b = {};", b2));
   BSON_ASSERT (bson_append_int32 (b, "2", -1, 1234));
   BSON_ASSERT (bson_append_int64 (b, "3", -1, 4567));
   BSON_ASSERT (bson_append_time_t (b, "4", -1, 123456));
   BSON_ASSERT (bson_append_decimal128 (b, "5", -1, &value));
   BSON_ASSERT (bson_iter_init (&iter, b));
   BSON_ASSERT (bson_iter_next (&iter));
   BSON_ASSERT (BSON_ITER_HOLDS_CODE (&iter));
   BSON_ASSERT (bson_iter_next (&iter));
   BSON_ASSERT (BSON_ITER_HOLDS_CODEWSCOPE (&iter));
   BSON_ASSERT (bson_iter_next (&iter));
   BSON_ASSERT (BSON_ITER_HOLDS_INT32 (&iter));
   BSON_ASSERT (bson_iter_next (&iter));
   BSON_ASSERT (BSON_ITER_HOLDS_INT64 (&iter));
   BSON_ASSERT (bson_iter_next (&iter));
   BSON_ASSERT (BSON_ITER_HOLDS_DATE_TIME (&iter));
   BSON_ASSERT (bson_iter_next (&iter));
   BSON_ASSERT (BSON_ITER_HOLDS_DECIMAL128 (&iter));
   BSON_ASSERT (!bson_iter_next (&iter));
   BSON_ASSERT (bson_iter_init_find (&iter, b, "3"));
   BSON_ASSERT (!strcmp (bson_iter_key (&iter), "3"));
   BSON_ASSERT (bson_iter_int64 (&iter) == 4567);
   BSON_ASSERT (bson_iter_next (&iter));
   BSON_ASSERT (BSON_ITER_HOLDS_DATE_TIME (&iter));
   BSON_ASSERT (bson_iter_time_t (&iter) == 123456);
   BSON_ASSERT (bson_iter_date_time (&iter) == 123456000);
   BSON_ASSERT (bson_iter_next (&iter));
   bson_iter_decimal128 (&iter, &iter_value);
   /* This test uses memcmp because libbson lacks decimal128 comparison. */
   BSON_ASSERT (memcmp (&iter_value, &value, sizeof (value)) == 0);
   BSON_ASSERT (!bson_iter_next (&iter));
   bson_destroy (b);
   bson_destroy (b2);
}


static void
test_bson_iter_overflow (void)
{
   bson_iter_t iter;
   bson_t *b;

   b = get_bson (BINARY_DIR "/overflow1.bson");
   BSON_ASSERT (!b);

   b = get_bson (BINARY_DIR "/overflow2.bson");
   BSON_ASSERT (b);
   BSON_ASSERT (bson_iter_init (&iter, b));
   BSON_ASSERT (!bson_iter_next (&iter));
   bson_destroy (b);
}


static void
test_bson_iter_binary_deprecated (void)
{
   bson_subtype_t subtype;
   uint32_t binary_len;
   const uint8_t *binary;
   bson_iter_t iter;
   bson_t *b;

   b = get_bson (BINARY_DIR "/binary_deprecated.bson");
   BSON_ASSERT (b);

   BSON_ASSERT (bson_iter_init (&iter, b));
   BSON_ASSERT (bson_iter_next (&iter));
   bson_iter_binary (&iter, &subtype, &binary_len, &binary);
   BSON_ASSERT (binary_len == 4);
   BSON_ASSERT (memcmp (binary, "1234", 4) == 0);

   bson_destroy (b);
}


static void
test_bson_iter_timeval (void)
{
   bson_iter_t iter;
   bson_t *b;
   struct timeval tv;

   b = get_bson (BINARY_DIR "/test26.bson");
   BSON_ASSERT (b);

   BSON_ASSERT (bson_iter_init (&iter, b));
   BSON_ASSERT (bson_iter_next (&iter));
   bson_iter_timeval (&iter, &tv);
   BSON_ASSERT (tv.tv_sec == 1234567890);
   BSON_ASSERT (tv.tv_usec == 0);

   bson_destroy (b);
}


static void
test_bson_iter_trailing_null (void)
{
   bson_iter_t iter;
   bson_t *b;

   b = get_bson (BINARY_DIR "/trailingnull.bson");
   BSON_ASSERT (b);
   BSON_ASSERT (bson_iter_init (&iter, b));
   BSON_ASSERT (!bson_iter_next (&iter));
   bson_destroy (b);
}


static void
test_bson_iter_fuzz (void)
{
   uint8_t *data;
   uint32_t len = 512;
   uint32_t len_le;
   uint32_t r;
   bson_iter_t iter;
   bson_t *b;
   uint32_t i;
   int pass;

   len_le = BSON_UINT32_TO_LE (len);

   for (pass = 0; pass < FUZZ_N_PASSES; pass++) {
      data = bson_malloc0 (len);
      memcpy (data, &len_le, sizeof (len_le));

      for (i = 4; i < len; i += 4) {
         r = rand ();
         memcpy (&data[i], &r, sizeof (r));
      }

      if (!(b = bson_new_from_data (data, len))) {
         /*
          * It could fail on buffer length or missing trailing null byte.
          */
         bson_free (data);
         continue;
      }

      BSON_ASSERT (b);

      /*
       * TODO: Most of the following ignores the key. That should be fixed
       *       but has it's own perils too.
       */

      BSON_ASSERT (bson_iter_init (&iter, b));
      while (bson_iter_next (&iter)) {
         BSON_ASSERT (iter.next_off < len);
         switch (bson_iter_type (&iter)) {
         case BSON_TYPE_ARRAY:
         case BSON_TYPE_DOCUMENT: {
            const uint8_t *child = NULL;
            uint32_t child_len = 0;

            bson_iter_document (&iter, &child_len, &child);
            if (child_len) {
               BSON_ASSERT (child);
               BSON_ASSERT (child_len >= 5);
               BSON_ASSERT ((iter.off + child_len) < b->len);
               BSON_ASSERT (child_len < (uint32_t) -1);
               memcpy (&child_len, child, sizeof (child_len));
               child_len = BSON_UINT32_FROM_LE (child_len);
               BSON_ASSERT (child_len >= 5);
            }
         } break;
         case BSON_TYPE_DOUBLE:
         case BSON_TYPE_UTF8:
         case BSON_TYPE_BINARY:
         case BSON_TYPE_UNDEFINED:
            break;
         case BSON_TYPE_OID:
            BSON_ASSERT (iter.off + 12 < iter.len);
            break;
         case BSON_TYPE_BOOL:
         case BSON_TYPE_DATE_TIME:
         case BSON_TYPE_NULL:
         case BSON_TYPE_REGEX:
         /* TODO: check for 2 valid cstring. */
         case BSON_TYPE_DBPOINTER:
         case BSON_TYPE_CODE:
         case BSON_TYPE_SYMBOL:
         case BSON_TYPE_CODEWSCOPE:
         case BSON_TYPE_INT32:
         case BSON_TYPE_TIMESTAMP:
         case BSON_TYPE_INT64:
         case BSON_TYPE_DECIMAL128:
         case BSON_TYPE_MAXKEY:
         case BSON_TYPE_MINKEY:
            break;
         case BSON_TYPE_EOD:
         default:
            /* Code should not be reached. */
            BSON_ASSERT (false);
            break;
         }
      }

      bson_destroy (b);
      bson_free (data);
   }
}


static void
test_bson_iter_regex (void)
{
   bson_iter_t iter;
   bson_t *b;

   b = bson_new ();
   BSON_ASSERT (bson_append_regex (b, "foo", -1, "^abcd", ""));
   BSON_ASSERT (bson_append_regex (b, "foo", -1, "^abcd", NULL));
   BSON_ASSERT (bson_append_regex (b, "foo", -1, "^abcd", "ix"));

   BSON_ASSERT (bson_iter_init (&iter, b));
   BSON_ASSERT (bson_iter_next (&iter));
   BSON_ASSERT (bson_iter_next (&iter));
   BSON_ASSERT (bson_iter_next (&iter));

   bson_destroy (b);
}


static void
test_bson_iter_next_after_finish (void)
{
   bson_iter_t iter;
   bson_t *b;
   int i;

   b = bson_new ();
   BSON_ASSERT (bson_append_int32 (b, "key", -1, 1234));
   BSON_ASSERT (bson_iter_init (&iter, b));
   BSON_ASSERT (bson_iter_next (&iter));
   for (i = 0; i < 1000; i++) {
      BSON_ASSERT (!bson_iter_next (&iter));
   }
   bson_destroy (b);
}


static void
test_bson_iter_find_case (void)
{
   bson_t b;
   bson_iter_t iter;

   bson_init (&b);
   BSON_ASSERT (bson_append_utf8 (&b, "key", -1, "value", -1));
   BSON_ASSERT (bson_iter_init (&iter, &b));
   BSON_ASSERT (bson_iter_find_case (&iter, "KEY"));
   BSON_ASSERT (bson_iter_init (&iter, &b));
   BSON_ASSERT (!bson_iter_find (&iter, "KEY"));
   bson_destroy (&b);
}


static void
test_bson_iter_as_double (void)
{
   bson_iter_t iter;
   bson_t b;

   bson_init (&b);
   ASSERT (bson_append_double (&b, "key", -1, 1234.1234));
   ASSERT (bson_iter_init_find (&iter, &b, "key"));
   ASSERT (BSON_ITER_HOLDS_DOUBLE (&iter));
   ASSERT (bson_iter_as_double (&iter) == 1234.1234);
   bson_destroy (&b);

   bson_init (&b);
   ASSERT (bson_append_int32 (&b, "key", -1, 1234));
   ASSERT (bson_iter_init_find (&iter, &b, "key"));
   ASSERT (BSON_ITER_HOLDS_INT32 (&iter));
   ASSERT (bson_iter_as_double (&iter) == 1234.0);
   bson_destroy (&b);

   bson_init (&b);
   ASSERT (bson_append_int64 (&b, "key", -1, 4321));
   ASSERT (bson_iter_init_find (&iter, &b, "key"));
   ASSERT (BSON_ITER_HOLDS_INT64 (&iter));
   ASSERT (bson_iter_as_double (&iter) == 4321.0);
   bson_destroy (&b);
}


static void
test_bson_iter_overwrite_int32 (void)
{
   bson_iter_t iter;
   bson_t b;

   bson_init (&b);
   BSON_ASSERT (bson_append_int32 (&b, "key", -1, 1234));
   BSON_ASSERT (bson_iter_init_find (&iter, &b, "key"));
   BSON_ASSERT (BSON_ITER_HOLDS_INT32 (&iter));
   bson_iter_overwrite_int32 (&iter, 4321);
   BSON_ASSERT (bson_iter_init_find (&iter, &b, "key"));
   BSON_ASSERT (BSON_ITER_HOLDS_INT32 (&iter));
   BSON_ASSERT_CMPINT (bson_iter_int32 (&iter), ==, 4321);
   bson_destroy (&b);
}


static void
test_bson_iter_overwrite_int64 (void)
{
   bson_iter_t iter;
   bson_t b;

   bson_init (&b);
   BSON_ASSERT (bson_append_int64 (&b, "key", -1, 1234));
   BSON_ASSERT (bson_iter_init_find (&iter, &b, "key"));
   BSON_ASSERT (BSON_ITER_HOLDS_INT64 (&iter));
   bson_iter_overwrite_int64 (&iter, 4641);
   BSON_ASSERT (bson_iter_init_find (&iter, &b, "key"));
   BSON_ASSERT (BSON_ITER_HOLDS_INT64 (&iter));
   BSON_ASSERT_CMPINT (bson_iter_int64 (&iter), ==, 4641);
   bson_destroy (&b);
}


static void
test_bson_iter_overwrite_decimal128 (void)
{
   bson_iter_t iter;
   bson_t b;
   bson_decimal128_t value;
   bson_decimal128_t new_value;
   bson_decimal128_t iter_value;

   value.high = 0;
   value.low = 1;

   new_value.high = 0;
   new_value.low = 2;

   bson_init (&b);
   BSON_ASSERT (bson_append_decimal128 (&b, "key", -1, &value));
   BSON_ASSERT (bson_iter_init_find (&iter, &b, "key"));
   BSON_ASSERT (BSON_ITER_HOLDS_DECIMAL128 (&iter));
   bson_iter_overwrite_decimal128 (&iter, &new_value);
   BSON_ASSERT (bson_iter_init_find (&iter, &b, "key"));
   BSON_ASSERT (BSON_ITER_HOLDS_DECIMAL128 (&iter));
   BSON_ASSERT (bson_iter_decimal128 (&iter, &iter_value));
   BSON_ASSERT (memcmp (&iter_value, &new_value, sizeof (new_value)) == 0);
   bson_destroy (&b);
}


static void
test_bson_iter_overwrite_double (void)
{
   bson_iter_t iter;
   bson_t b;

   bson_init (&b);
   BSON_ASSERT (bson_append_double (&b, "key", -1, 1234.1234));
   BSON_ASSERT (bson_iter_init_find (&iter, &b, "key"));
   BSON_ASSERT (BSON_ITER_HOLDS_DOUBLE (&iter));
   bson_iter_overwrite_double (&iter, 4641.1234);
   BSON_ASSERT (bson_iter_init_find (&iter, &b, "key"));
   BSON_ASSERT (BSON_ITER_HOLDS_DOUBLE (&iter));
   BSON_ASSERT_CMPINT (bson_iter_double (&iter), ==, 4641.1234);
   bson_destroy (&b);
}


static void
test_bson_iter_overwrite_bool (void)
{
   bson_iter_t iter;
   bson_t b;

   bson_init (&b);
   BSON_ASSERT (bson_append_bool (&b, "key", -1, true));
   BSON_ASSERT (bson_iter_init_find (&iter, &b, "key"));
   BSON_ASSERT (BSON_ITER_HOLDS_BOOL (&iter));
   bson_iter_overwrite_bool (&iter, false);
   BSON_ASSERT (bson_iter_init_find (&iter, &b, "key"));
   BSON_ASSERT (BSON_ITER_HOLDS_BOOL (&iter));
   BSON_ASSERT_CMPINT (bson_iter_bool (&iter), ==, false);
   bson_destroy (&b);
}


static void
test_bson_iter_recurse (void)
{
   bson_iter_t iter;
   bson_iter_t child;
   bson_t b;
   bson_t cb;

   bson_init (&b);
   bson_init (&cb);
   BSON_ASSERT (bson_append_int32 (&cb, "0", 1, 0));
   BSON_ASSERT (bson_append_int32 (&cb, "1", 1, 1));
   BSON_ASSERT (bson_append_int32 (&cb, "2", 1, 2));
   BSON_ASSERT (bson_append_array (&b, "key", -1, &cb));
   BSON_ASSERT (bson_iter_init_find (&iter, &b, "key"));
   BSON_ASSERT (BSON_ITER_HOLDS_ARRAY (&iter));
   BSON_ASSERT (bson_iter_recurse (&iter, &child));
   BSON_ASSERT (bson_iter_find (&child, "0"));
   BSON_ASSERT (bson_iter_find (&child, "1"));
   BSON_ASSERT (bson_iter_find (&child, "2"));
   BSON_ASSERT (!bson_iter_next (&child));
   bson_destroy (&b);
   bson_destroy (&cb);
}


static void
test_bson_iter_init_find_case (void)
{
   bson_t b;
   bson_iter_t iter;

   bson_init (&b);
   BSON_ASSERT (bson_append_int32 (&b, "FOO", -1, 1234));
   BSON_ASSERT (bson_iter_init_find_case (&iter, &b, "foo"));
   BSON_ASSERT_CMPINT (bson_iter_int32 (&iter), ==, 1234);
   bson_destroy (&b);
}


static void
test_bson_iter_find_descendant (void)
{
   bson_iter_t iter;
   bson_iter_t desc;
   bson_t *b;

   b = get_bson (BINARY_DIR "/dotkey.bson");
   BSON_ASSERT (bson_iter_init (&iter, b));
   BSON_ASSERT (bson_iter_find_descendant (&iter, "a.b.c.0", &desc));
   BSON_ASSERT (BSON_ITER_HOLDS_INT32 (&desc));
   BSON_ASSERT (bson_iter_int32 (&desc) == 1);
   bson_destroy (b);

   b = BCON_NEW (
      "foo", "{", "bar", "[", "{", "baz", BCON_INT32 (1), "}", "]", "}");
   BSON_ASSERT (bson_iter_init (&iter, b));
   BSON_ASSERT (bson_iter_find_descendant (&iter, "foo.bar.0.baz", &desc));
   BSON_ASSERT (BSON_ITER_HOLDS_INT32 (&desc));
   BSON_ASSERT (bson_iter_int32 (&desc) == 1);
   bson_destroy (b);

   b = BCON_NEW ("nModified", BCON_INT32 (1), "n", BCON_INT32 (2));
   BSON_ASSERT (bson_iter_init (&iter, b));
   BSON_ASSERT (bson_iter_find_descendant (&iter, "n", &desc));
   BSON_ASSERT (!strcmp (bson_iter_key (&desc), "n"));
   bson_destroy (b);

   b = BCON_NEW ("", BCON_INT32 (1), "n", BCON_INT32 (2));
   BSON_ASSERT (bson_iter_init (&iter, b));
   BSON_ASSERT (bson_iter_find_descendant (&iter, "n", &desc));
   BSON_ASSERT (!strcmp (bson_iter_key (&desc), "n"));
   bson_destroy (b);
}


static void
test_bson_iter_as_bool (void)
{
   bson_iter_t iter;
   bson_t b;

   bson_init (&b);
   bson_append_int32 (&b, "int32[true]", -1, 1);
   bson_append_int32 (&b, "int32[false]", -1, 0);
   bson_append_int64 (&b, "int64[true]", -1, 1);
   bson_append_int64 (&b, "int64[false]", -1, 0);
   bson_append_double (&b, "int64[true]", -1, 1.0);
   bson_append_double (&b, "int64[false]", -1, 0.0);

   bson_iter_init (&iter, &b);
   bson_iter_next (&iter);
   BSON_ASSERT_CMPINT (true, ==, bson_iter_as_bool (&iter));
   bson_iter_next (&iter);
   BSON_ASSERT_CMPINT (false, ==, bson_iter_as_bool (&iter));
   bson_iter_next (&iter);
   BSON_ASSERT_CMPINT (true, ==, bson_iter_as_bool (&iter));
   bson_iter_next (&iter);
   BSON_ASSERT_CMPINT (false, ==, bson_iter_as_bool (&iter));
   bson_iter_next (&iter);
   BSON_ASSERT_CMPINT (true, ==, bson_iter_as_bool (&iter));
   bson_iter_next (&iter);
   BSON_ASSERT_CMPINT (false, ==, bson_iter_as_bool (&iter));

   bson_destroy (&b);
}

static void
test_bson_iter_from_data (void)
{
   /* {"b": true}, with implicit NULL at end */
   uint8_t data[] = "\x09\x00\x00\x00\x08\x62\x00\x01";
   bson_iter_t iter;

   ASSERT (bson_iter_init_from_data (&iter, data, sizeof data));
   ASSERT (bson_iter_next (&iter));
   ASSERT (BSON_ITER_HOLDS_BOOL (&iter));
   ASSERT (bson_iter_bool (&iter));
}

void
test_iter_install (TestSuite *suite)
{
   TestSuite_Add (suite, "/bson/iter/test_string", test_bson_iter_utf8);
   TestSuite_Add (suite, "/bson/iter/test_mixed", test_bson_iter_mixed);
   TestSuite_Add (suite, "/bson/iter/test_overflow", test_bson_iter_overflow);
   TestSuite_Add (suite, "/bson/iter/test_timeval", test_bson_iter_timeval);
   TestSuite_Add (
      suite, "/bson/iter/test_trailing_null", test_bson_iter_trailing_null);
   TestSuite_Add (suite, "/bson/iter/test_fuzz", test_bson_iter_fuzz);
   TestSuite_Add (suite, "/bson/iter/test_regex", test_bson_iter_regex);
   TestSuite_Add (suite,
                  "/bson/iter/test_next_after_finish",
                  test_bson_iter_next_after_finish);
   TestSuite_Add (suite, "/bson/iter/test_find_case", test_bson_iter_find_case);
   TestSuite_Add (
      suite, "/bson/iter/test_bson_iter_as_double", test_bson_iter_as_double);
   TestSuite_Add (
      suite, "/bson/iter/test_overwrite_int32", test_bson_iter_overwrite_int32);
   TestSuite_Add (
      suite, "/bson/iter/test_overwrite_int64", test_bson_iter_overwrite_int64);
   TestSuite_Add (suite,
                  "/bson/iter/test_overwrite_double",
                  test_bson_iter_overwrite_double);
   TestSuite_Add (
      suite, "/bson/iter/test_overwrite_bool", test_bson_iter_overwrite_bool);
   TestSuite_Add (suite,
                  "/bson/iter/test_bson_iter_overwrite_decimal128",
                  test_bson_iter_overwrite_decimal128);
   TestSuite_Add (suite, "/bson/iter/recurse", test_bson_iter_recurse);
   TestSuite_Add (
      suite, "/bson/iter/init_find_case", test_bson_iter_init_find_case);
   TestSuite_Add (
      suite, "/bson/iter/find_descendant", test_bson_iter_find_descendant);
   TestSuite_Add (suite, "/bson/iter/as_bool", test_bson_iter_as_bool);
   TestSuite_Add (
      suite, "/bson/iter/binary_deprecated", test_bson_iter_binary_deprecated);
   TestSuite_Add (suite, "/bson/iter/from_data", test_bson_iter_from_data);
}
