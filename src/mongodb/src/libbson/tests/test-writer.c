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


#include <time.h>

#include "bson-tests.h"
#include "TestSuite.h"


static void *
test_bson_writer_custom_realloc_helper (void *mem, size_t num_bytes, void *ctx)
{
   int *x = (int *) ctx;

   *x = *x + 1;
   return bson_realloc (mem, num_bytes);
}

static void
test_bson_writer_custom_realloc (void)
{
   bson_writer_t *writer;
   uint8_t *buf = bson_malloc (0);
   size_t buflen = 0;
   bson_t *b;
   int x = 0;

   writer = bson_writer_new (
      &buf, &buflen, 0, test_bson_writer_custom_realloc_helper, &x);

   BSON_ASSERT (bson_writer_begin (writer, &b));

   BSON_ASSERT (bson_append_utf8 (b, "hello", -1, "world", -1));

   bson_writer_end (writer);

   bson_writer_destroy (writer);

   BSON_ASSERT_CMPINT (x, >, 0);

   bson_free (buf);
}


static void
test_bson_writer_shared_buffer (void)
{
   bson_writer_t *writer;
   uint8_t *buf = bson_malloc0 (32);
   bool rolled_back = false;
   size_t buflen = 32;
   size_t n_bytes;
   bson_t *b;
   const char *key;
   char keystr[32];
   int i = 0;
   int j = 0;
   int n_docs = 10000;

   writer = bson_writer_new (&buf, &buflen, 0, bson_realloc_ctx, NULL);

   for (i = 0; i < n_docs; i++) {
      BSON_ASSERT (bson_writer_begin (writer, &b));

      for (j = 0; j < 1000; j++) {
         bson_uint32_to_string (j, &key, keystr, sizeof keystr);
         BSON_ASSERT (bson_append_int64 (b, key, -1, j));
      }

      if (bson_writer_get_length (writer) > (48 * 1024 * 1024)) {
         rolled_back = true;
         bson_writer_rollback (writer);
         break;
      } else {
         bson_writer_end (writer);
      }
   }

   n_bytes = bson_writer_get_length (writer);

   bson_writer_destroy (writer);

   BSON_ASSERT_CMPINT (n_bytes, <, (48 * 1024 * 1024));
   BSON_ASSERT_CMPINT (rolled_back, ==, true);
   BSON_ASSERT (rolled_back);

   bson_free (buf);
}


static void
test_bson_writer_empty_sequence (void)
{
   const uint8_t testdata[] = {5, 0, 0, 0, 0, 5, 0, 0, 0, 0, 5, 0, 0,
                               0, 0, 5, 0, 0, 0, 0, 5, 0, 0, 0, 0};
   bson_writer_t *writer;
   uint8_t *buf = NULL;
   bson_t *b;
   size_t len = 0;
   int r;
   int i;

   writer = bson_writer_new (&buf, &len, 0, bson_realloc_ctx, NULL);
   for (i = 0; i < 5; i++) {
      BSON_ASSERT (bson_writer_begin (writer, &b));
      bson_writer_end (writer);
   }
   r = memcmp (buf, testdata, 25);
   BSON_ASSERT (r == 0);
   bson_writer_destroy (writer);
   bson_free (buf);
}


static void
test_bson_writer_null_realloc (void)
{
   const uint8_t testdata[] = {5, 0, 0, 0, 0, 5, 0, 0, 0, 0, 5, 0, 0, 0, 0, 5,
                               0, 0, 0, 0, 5, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0};
   bson_writer_t *writer;
   uint8_t *buf = bson_malloc0 (32);
   size_t buflen = 32;
   bson_t *b;
   int r;
   int i;

   writer = bson_writer_new (&buf, &buflen, 0, NULL, NULL);
   for (i = 0; i < 6; i++) {
      BSON_ASSERT (bson_writer_begin (writer, &b));
      bson_writer_end (writer);
   }

   BSON_ASSERT (!bson_writer_begin (writer, &b));

   r = memcmp (buf, testdata, 32);
   BSON_ASSERT (r == 0);
   bson_writer_destroy (writer);

   bson_free (buf);
}

static void
test_bson_writer_null_realloc_2 (void)
{
   const uint8_t testdata[] = {5, 0, 0, 0, 0, 5, 0, 0, 0, 0, 5, 0, 0, 0, 0, 5,
                               0, 0, 0, 0, 5, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0};
   bson_writer_t *writer;
   uint8_t *buf = bson_malloc0 (32);
   size_t buflen = 32;
   bson_t *b;
   int r;
   int i;

   writer = bson_writer_new (&buf, &buflen, 0, NULL, NULL);
   for (i = 0; i < 5; i++) {
      BSON_ASSERT (bson_writer_begin (writer, &b));
      bson_writer_end (writer);
   }

   BSON_ASSERT (bson_writer_begin (writer, &b));
   BSON_ASSERT (!bson_append_int32 (b, "a", -1, 123));
   bson_writer_end (writer);

   r = memcmp (buf, testdata, 32);
   BSON_ASSERT (r == 0);
   bson_writer_destroy (writer);

   bson_free (buf);
}

void
test_writer_install (TestSuite *suite)
{
   TestSuite_Add (
      suite, "/bson/writer/custom_realloc", test_bson_writer_custom_realloc);
   TestSuite_Add (
      suite, "/bson/writer/shared_buffer", test_bson_writer_shared_buffer);
   TestSuite_Add (
      suite, "/bson/writer/empty_sequence", test_bson_writer_empty_sequence);
   TestSuite_Add (
      suite, "/bson/writer/null_realloc", test_bson_writer_null_realloc);
   TestSuite_Add (
      suite, "/bson/writer/null_realloc_2", test_bson_writer_null_realloc_2);
}
