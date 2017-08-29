/*
 * Copyright 2016 MongoDB, Inc.
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

#include "TestSuite.h"
#include "corpus-test.h"


#ifdef _MSC_VER
#define SSCANF sscanf_s
#else
#define SSCANF sscanf
#endif

void
corpus_test_print_description (const char *description)
{
   if (test_suite_debug_output ()) {
      printf ("  - %s\n", description);
      fflush (stdout);
   }
}

uint8_t *
corpus_test_unhexlify (bson_iter_t *iter, uint32_t *bson_str_len)
{
   const char *hex_str;
   uint8_t *data = NULL;
   unsigned int byte;
   uint32_t tmp;
   int x = 0;
   int i = 0;

   ASSERT (BSON_ITER_HOLDS_UTF8 (iter));
   hex_str = bson_iter_utf8 (iter, &tmp);
   *bson_str_len = tmp / 2;
   data = bson_malloc (*bson_str_len);
   while (SSCANF (&hex_str[i], "%2x", &byte) == 1) {
      data[x++] = (uint8_t) byte;
      i += 2;
   }

   return data;
}


void
corpus_test (bson_t *scenario,
             test_bson_valid_cb valid,
             test_bson_decode_error_cb decode_error,
             test_bson_parse_error_cb parse_error)
{
   bson_iter_t iter;
   bson_iter_t inner_iter;
   const char *scenario_description = NULL;
   bson_type_t bson_type;

   BSON_ASSERT (scenario);

   BSON_ASSERT (bson_iter_init_find (&iter, scenario, "description"));
   scenario_description = bson_iter_utf8 (&iter, NULL);

   BSON_ASSERT (bson_iter_init_find (&iter, scenario, "bson_type"));
   /* like "0x0C */
   if (sscanf (bson_iter_utf8 (&iter, NULL), "%i", (int *) &bson_type) != 1) {
      fprintf (
         stderr, "Couldn't parse bson_type %s\n", bson_iter_utf8 (&iter, NULL));
      abort ();
   }

   /* test valid BSON and Extended JSON */
   if (bson_iter_init_find (&iter, scenario, "valid")) {
      bson_iter_recurse (&iter, &inner_iter);
      while (bson_iter_next (&inner_iter)) {
         bson_iter_t test_iter;
         test_bson_valid_type_t test = {scenario_description, bson_type, NULL};

         bson_iter_recurse (&inner_iter, &test_iter);
         while (bson_iter_next (&test_iter)) {
            const char *key = bson_iter_key (&test_iter);

            if (!strcmp (key, "description")) {
               test.test_description = bson_iter_utf8 (&test_iter, NULL);
               corpus_test_print_description (test.test_description);
            }

            if (!strcmp (key, "canonical_bson")) {
               test.cB = corpus_test_unhexlify (&test_iter, &test.cB_len);
            }

            if (!strcmp (key, "degenerate_bson")) {
               test.dB = corpus_test_unhexlify (&test_iter, &test.dB_len);
            }

            if (!strcmp (key, "canonical_extjson")) {
               test.cE = bson_iter_utf8 (&test_iter, NULL);
            }

            if (!strcmp (key, "degenerate_extjson")) {
               test.dE = bson_iter_utf8 (&test_iter, NULL);
            }

            if (!strcmp (key, "relaxed_extjson")) {
               test.rE = bson_iter_utf8 (&test_iter, NULL);
            }

            if (!strcmp (key, "lossy")) {
               test.lossy = bson_iter_bool (&test_iter);
            }
         }

         /* execute the test callback */
         valid (&test);

         bson_free (test.cB);
         bson_free (test.dB);
      }
   }

   /* test decoding errors (i.e. invalid BSON) */
   if (bson_iter_init_find (&iter, scenario, "decodeErrors")) {
      bson_iter_recurse (&iter, &inner_iter);
      while (bson_iter_next (&inner_iter)) {
         bson_iter_t test_iter;
         test_bson_decode_error_type_t test = {
            scenario_description, bson_type, NULL};

         bson_iter_recurse (&inner_iter, &test_iter);
         while (bson_iter_next (&test_iter)) {
            if (!strcmp (bson_iter_key (&test_iter), "description")) {
               test.test_description = bson_iter_utf8 (&test_iter, NULL);
               corpus_test_print_description (test.test_description);
            }

            if (!strcmp (bson_iter_key (&test_iter), "bson")) {
               test.bson = corpus_test_unhexlify (&test_iter, &test.bson_len);
            }
         }

         /* execute the test callback */
         decode_error (&test);

         bson_free (test.bson);
      }
   }

   /* test parsing errors (e.g. invalid JSON or Decimal128 strings) */
   if (bson_iter_init_find (&iter, scenario, "parseErrors")) {
      bson_iter_recurse (&iter, &inner_iter);
      while (bson_iter_next (&inner_iter)) {
         bson_iter_t test_iter;
         test_bson_parse_error_type_t test = {
            scenario_description, bson_type, NULL};

         bson_iter_recurse (&inner_iter, &test_iter);
         while (bson_iter_next (&test_iter)) {
            if (!strcmp (bson_iter_key (&test_iter), "description")) {
               test.test_description = bson_iter_utf8 (&test_iter, NULL);
               corpus_test_print_description (test.test_description);
            }

            if (!strcmp (bson_iter_key (&test_iter), "string")) {
               test.str = bson_iter_utf8 (&test_iter, &test.str_len);
            }
         }

         /* execute the test callback */
         parse_error (&test);
      }
   }
}
