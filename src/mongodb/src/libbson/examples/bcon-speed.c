/*
 * Copyright 2013-2014 MongoDB, Inc.
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
#include <stdio.h>
#include <stdlib.h>

/*
 * This is a test for comparing the performance of BCON to regular
 * bson_append*() function calls.
 *
 * Maybe run the following a few times to get an idea of the performance
 * implications of using BCON. Generally, it's fast enough to be very
 * useful and result in easier to read BSON code.
 *
 * time ./bcon-speed 100000 y
 * time ./bcon-speed 100000 n
 */


int
main (int argc, char *argv[])
{
   int i;
   int n;
   int bcon;
   bson_t bson, foo, bar, baz;
   bson_init (&bson);

   if (argc != 3) {
      fprintf (stderr,
               "usage: bcon-speed NUM_ITERATIONS [y|n]\n"
               "\n"
               "  y = perform speed tests with bcon\n"
               "  n = perform speed tests with bson_append\n"
               "\n");
      return EXIT_FAILURE;
   }

   BSON_ASSERT (argc == 3);

   n = atoi (argv[1]);
   bcon = (argv[2][0] == 'y') ? 1 : 0;

   for (i = 0; i < n; i++) {
      if (bcon) {
         BCON_APPEND (&bson,
                      "foo",
                      "{",
                      "bar",
                      "{",
                      "baz",
                      "[",
                      BCON_INT32 (1),
                      BCON_INT32 (2),
                      BCON_INT32 (3),
                      "]",
                      "}",
                      "}");
      } else {
         bson_append_document_begin (&bson, "foo", -1, &foo);
         bson_append_document_begin (&foo, "bar", -1, &bar);
         bson_append_array_begin (&bar, "baz", -1, &baz);
         bson_append_int32 (&baz, "0", -1, 1);
         bson_append_int32 (&baz, "1", -1, 2);
         bson_append_int32 (&baz, "2", -1, 3);
         bson_append_array_end (&bar, &baz);
         bson_append_document_end (&foo, &bar);
         bson_append_document_end (&bson, &foo);
      }

      bson_reinit (&bson);
   }

   bson_destroy (&bson);

   return 0;
}
