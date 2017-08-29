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
#define BSON_INSIDE
#include "bson-thread-private.h"
#undef BSON_INSIDE
#include <ctype.h>
#include <fcntl.h>
#include <time.h>

#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif

#include "bson-tests.h"
#include "TestSuite.h"

#define N_THREADS 4

static const char *gTestOids[] = {"000000000000000000000000",
                                  "010101010101010101010101",
                                  "0123456789abcdefafcdef03",
                                  "fcdeab182763817236817236",
                                  "ffffffffffffffffffffffff",
                                  "eeeeeeeeeeeeeeeeeeeeeeee",
                                  "999999999999999999999999",
                                  "111111111111111111111111",
                                  NULL};

static const char *gTestOidsCase[] = {"0123456789ABCDEFAFCDEF03",
                                      "FCDEAB182763817236817236",
                                      "FFFFFFFFFFFFFFFFFFFFFFFF",
                                      "EEEEEEEEEEEEEEEEEEEEEEEE",
                                      "01234567890ACBCDEFabcdef",
                                      NULL};

static const char *gTestOidsFail[] = {"                        ",
                                      "abasdf                  ",
                                      "asdfasdfasdfasdfasdf    ",
                                      "00000000000000000000000z",
                                      "00187263123ghh21382812a8",
                                      NULL};

static void *
oid_worker (void *data)
{
   bson_context_t *context = data;
   bson_oid_t oid;
   bson_oid_t oid2;
   int i;

   bson_oid_init (&oid2, context);
   for (i = 0; i < 500000; i++) {
      bson_oid_init (&oid, context);
      BSON_ASSERT (false == bson_oid_equal (&oid, &oid2));
      BSON_ASSERT (0 < bson_oid_compare (&oid, &oid2));
      bson_oid_copy (&oid, &oid2);
   }

   return NULL;
}

static void
test_bson_oid_init_from_string (void)
{
   bson_context_t *context;
   bson_oid_t oid;
   char str[25];
   int i;

   context = bson_context_new (BSON_CONTEXT_NONE);

   /*
    * Test successfully parsed oids.
    */

   for (i = 0; gTestOids[i]; i++) {
      bson_oid_init_from_string (&oid, gTestOids[i]);
      bson_oid_to_string (&oid, str);
      BSON_ASSERT (!strcmp (str, gTestOids[i]));
   }

   /*
    * Test successfully parsed oids (case-insensitive).
    */
   for (i = 0; gTestOidsCase[i]; i++) {
      char oid_lower[25];
      int j;

      bson_oid_init_from_string (&oid, gTestOidsCase[i]);
      bson_oid_to_string (&oid, str);
      BSON_ASSERT (!bson_strcasecmp (str, gTestOidsCase[i]));

      for (j = 0; gTestOidsCase[i][j]; j++) {
         oid_lower[j] = tolower (gTestOidsCase[i][j]);
      }

      oid_lower[24] = '\0';
      BSON_ASSERT (!strcmp (str, oid_lower));
   }

   /*
    * Test that sizeof(str) works (len of 25 with \0 instead of 24).
    */
   BSON_ASSERT (bson_oid_is_valid ("ffffffffffffffffffffffff", 24));
   bson_oid_init_from_string (&oid, "ffffffffffffffffffffffff");
   bson_oid_to_string (&oid, str);
   BSON_ASSERT (bson_oid_is_valid (str, sizeof str));

   /*
    * Test the failures.
    */

   for (i = 0; gTestOidsFail[i]; i++) {
      bson_oid_init_from_string (&oid, gTestOidsFail[i]);
      bson_oid_to_string (&oid, str);
      BSON_ASSERT (strcmp (str, gTestOidsFail[i]));
   }

   bson_context_destroy (context);
}


static void
test_bson_oid_hash (void)
{
   bson_oid_t oid;

   bson_oid_init_from_string (&oid, "000000000000000000000000");
   BSON_ASSERT (bson_oid_hash (&oid) == 1487062149);
}


static void
test_bson_oid_compare (void)
{
   bson_oid_t oid;
   bson_oid_t oid2;

   bson_oid_init_from_string (&oid, "000000000000000000001234");
   bson_oid_init_from_string (&oid2, "000000000000000000001234");
   BSON_ASSERT (0 == bson_oid_compare (&oid, &oid2));
   BSON_ASSERT (true == bson_oid_equal (&oid, &oid2));

   bson_oid_init_from_string (&oid, "000000000000000000001234");
   bson_oid_init_from_string (&oid2, "000000000000000000004321");
   BSON_ASSERT (bson_oid_compare (&oid, &oid2) < 0);
   BSON_ASSERT (bson_oid_compare (&oid2, &oid) > 0);
   BSON_ASSERT (false == bson_oid_equal (&oid, &oid2));
}


static void
test_bson_oid_copy (void)
{
   bson_oid_t oid;
   bson_oid_t oid2;

   bson_oid_init_from_string (&oid, "000000000000000000001234");
   bson_oid_init_from_string (&oid2, "000000000000000000004321");
   bson_oid_copy (&oid, &oid2);
   BSON_ASSERT (true == bson_oid_equal (&oid, &oid2));
}


static void
test_bson_oid_init (void)
{
   bson_context_t *context;
   bson_oid_t oid;
   bson_oid_t oid2;
   int i;

   context = bson_context_new (BSON_CONTEXT_NONE);
   bson_oid_init (&oid, context);
   for (i = 0; i < 10000; i++) {
      bson_oid_init (&oid2, context);
      BSON_ASSERT (false == bson_oid_equal (&oid, &oid2));
      BSON_ASSERT (0 > bson_oid_compare (&oid, &oid2));
      bson_oid_copy (&oid2, &oid);
   }
   bson_context_destroy (context);

   /*
    * Test that the shared context works.
    */
   bson_oid_init (&oid, NULL);
   BSON_ASSERT (bson_context_get_default ());
}


static void
test_bson_oid_init_sequence (void)
{
   bson_context_t *context;
   bson_oid_t oid;
   bson_oid_t oid2;
   int i;

   context = bson_context_new (BSON_CONTEXT_NONE);
   bson_oid_init_sequence (&oid, context);
   for (i = 0; i < 10000; i++) {
      bson_oid_init_sequence (&oid2, context);
      BSON_ASSERT (false == bson_oid_equal (&oid, &oid2));
      BSON_ASSERT (0 > bson_oid_compare (&oid, &oid2));
      bson_oid_copy (&oid2, &oid);
   }
   bson_context_destroy (context);
}


static void
test_bson_oid_init_sequence_thread_safe (void)
{
   bson_context_t *context;
   bson_oid_t oid;
   bson_oid_t oid2;
   int i;

   context = bson_context_new (BSON_CONTEXT_THREAD_SAFE);
   bson_oid_init_sequence (&oid, context);
   for (i = 0; i < 10000; i++) {
      bson_oid_init_sequence (&oid2, context);
      BSON_ASSERT (false == bson_oid_equal (&oid, &oid2));
      BSON_ASSERT (0 > bson_oid_compare (&oid, &oid2));
      bson_oid_copy (&oid2, &oid);
   }
   bson_context_destroy (context);
}


#ifdef BSON_HAVE_SYSCALL_TID
static void
test_bson_oid_init_sequence_with_tid (void)
{
   bson_context_t *context;
   bson_oid_t oid;
   bson_oid_t oid2;
   int i;

   context = bson_context_new (BSON_CONTEXT_USE_TASK_ID);
   bson_oid_init_sequence (&oid, context);
   for (i = 0; i < 10000; i++) {
      bson_oid_init_sequence (&oid2, context);
      BSON_ASSERT (false == bson_oid_equal (&oid, &oid2));
      BSON_ASSERT (0 > bson_oid_compare (&oid, &oid2));
      bson_oid_copy (&oid2, &oid);
   }
   bson_context_destroy (context);
}
#endif


static void
test_bson_oid_get_time_t (void)
{
   bson_context_t *context;
   bson_oid_t oid;
   bson_oid_t oid2;

   /*
    * Test that the bson time_t matches the current time. This can race, but
    * i dont think that matters much.
    */
   context = bson_context_new (BSON_CONTEXT_NONE);
   bson_oid_init (&oid, context);
   bson_oid_init (&oid2, context);
   BSON_ASSERT (bson_oid_get_time_t (&oid) == bson_oid_get_time_t (&oid2));
   BSON_ASSERT (time (NULL) == bson_oid_get_time_t (&oid2));
   bson_context_destroy (context);
}


static void
test_bson_oid_init_with_threads (void)
{
   bson_context_t *context;
   int i;

   {
      bson_context_flags_t flags = 0;
      bson_context_t *contexts[N_THREADS];
      bson_thread_t threads[N_THREADS];

#ifdef BSON_HAVE_SYSCALL_TID
      flags |= BSON_CONTEXT_USE_TASK_ID;
#endif

      for (i = 0; i < N_THREADS; i++) {
         contexts[i] = bson_context_new (flags);
         bson_thread_create (&threads[i], oid_worker, contexts[i]);
      }

      for (i = 0; i < N_THREADS; i++) {
         bson_thread_join (threads[i]);
      }

      for (i = 0; i < N_THREADS; i++) {
         bson_context_destroy (contexts[i]);
      }
   }

   /*
    * Test threaded generation of oids using a single context;
    */
   {
      bson_thread_t threads[N_THREADS];

      context = bson_context_new (BSON_CONTEXT_THREAD_SAFE);

      for (i = 0; i < N_THREADS; i++) {
         bson_thread_create (&threads[i], oid_worker, context);
      }

      for (i = 0; i < N_THREADS; i++) {
         bson_thread_join (threads[i]);
      }

      bson_context_destroy (context);
   }
}

void
test_oid_install (TestSuite *suite)
{
   TestSuite_Add (suite, "/bson/oid/init", test_bson_oid_init);
   TestSuite_Add (
      suite, "/bson/oid/init_from_string", test_bson_oid_init_from_string);
   TestSuite_Add (
      suite, "/bson/oid/init_sequence", test_bson_oid_init_sequence);
   TestSuite_Add (suite,
                  "/bson/oid/init_sequence_thread_safe",
                  test_bson_oid_init_sequence_thread_safe);
#ifdef BSON_HAVE_SYSCALL_TID
   TestSuite_Add (suite,
                  "/bson/oid/init_sequence_with_tid",
                  test_bson_oid_init_sequence_with_tid);
#endif
   TestSuite_Add (
      suite, "/bson/oid/init_with_threads", test_bson_oid_init_with_threads);
   TestSuite_Add (suite, "/bson/oid/hash", test_bson_oid_hash);
   TestSuite_Add (suite, "/bson/oid/compare", test_bson_oid_compare);
   TestSuite_Add (suite, "/bson/oid/copy", test_bson_oid_copy);
   TestSuite_Add (suite, "/bson/oid/get_time_t", test_bson_oid_get_time_t);
}
