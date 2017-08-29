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

#define QUERY(...) ELE_QUERY, __VA_ARGS__, NULL
#define SORT(...) ELE_SORT, __VA_ARGS__, NULL
#define LIMIT(var) ELE_LIMIT, (var)
#define COL_VIEW_CREATE(...) col_view_create ("", __VA_ARGS__, ELE_END)

typedef enum ele {
   ELE_SORT,
   ELE_LIMIT,
   ELE_QUERY,
   ELE_END,
} ele_t;

bson_t *
col_view_create (const char *stub, ...)
{
   bson_t *bson;
   va_list ap;
   ele_t type;
   int keep_going = 1;

   bcon_append_ctx_t ctx;
   bcon_append_ctx_init (&ctx);

   va_start (ap, stub);

   bson = bson_new ();

   while (keep_going) {
      type = va_arg (ap, ele_t);

      switch (type) {
      case ELE_SORT:
         BCON_APPEND_CTX (bson, &ctx, "sort", "{");
         bcon_append_ctx_va (bson, &ctx, &ap);
         BCON_APPEND_CTX (bson, &ctx, "}");
         break;
      case ELE_LIMIT: {
         int i = va_arg (ap, int);
         BCON_APPEND_CTX (bson, &ctx, "limit", BCON_INT32 (i));
         break;
      }
      case ELE_QUERY:
         BCON_APPEND_CTX (bson, &ctx, "query", "{");
         bcon_append_ctx_va (bson, &ctx, &ap);
         BCON_APPEND_CTX (bson, &ctx, "}");
         break;
      case ELE_END:
         keep_going = 0;
         break;
      default:
         BSON_ASSERT (0);
         break;
      }
   }

   va_end (ap);

   return bson;
}

int
main (int argc, char *argv[])
{
   bson_t *bson;
   char *json;

   bson = COL_VIEW_CREATE (
      SORT ("a", BCON_INT32 (1)), QUERY ("hello", "world"), LIMIT (10));

   json = bson_as_canonical_extended_json (bson, NULL);
   printf ("%s\n", json);
   bson_free (json);

   bson_destroy (bson);

   return 0;
}
