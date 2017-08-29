/* Copyright 2017 MongoDB Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* -- sphinx-include-start -- */
#include <stdio.h>
#include <bson.h>

int
main (int argc, const char **argv)
{
   bson_t *b;
   char *j;

   b = BCON_NEW ("hello", BCON_UTF8 ("bson!"));
   j = bson_as_canonical_extended_json (b, NULL);
   printf ("%s\n", j);

   bson_free (j);
   bson_destroy (b);

   return 0;
}
