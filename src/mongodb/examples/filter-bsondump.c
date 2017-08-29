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
#include <mongoc.h>
#include <stdio.h>


/*
 * This is an example that reads BSON documents from STDIN and prints them
 * to standard output as JSON if they match {'hello': 'world'}.
 */


int
main (int argc, char *argv[])
{
   mongoc_matcher_t *matcher;
   bson_reader_t *reader;
   const bson_t *bson;
   bson_t *spec;
   char *str;
   int fd;

   mongoc_init ();

#ifdef _WIN32
   fd = fileno (stdin);
#else
   fd = STDIN_FILENO;
#endif

   reader = bson_reader_new_from_fd (fd, false);

   spec = BCON_NEW ("hello", "world");
   matcher = mongoc_matcher_new (spec, NULL);

   while ((bson = bson_reader_read (reader, NULL))) {
      if (mongoc_matcher_match (matcher, bson)) {
         str = bson_as_canonical_extended_json (bson, NULL);
         printf ("%s\n", str);
         bson_free (str);
      }
   }

   bson_reader_destroy (reader);
   bson_destroy (spec);

   return 0;
}
