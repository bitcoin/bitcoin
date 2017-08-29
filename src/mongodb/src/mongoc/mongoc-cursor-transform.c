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


#include "mongoc-cursor.h"
#include "mongoc-cursor-transform-private.h"
#include "mongoc-cursor-private.h"
#include "mongoc-client-private.h"
#include "mongoc-counters-private.h"
#include "mongoc-error.h"
#include "mongoc-log.h"
#include "mongoc-opcode.h"
#include "mongoc-trace-private.h"


#undef MONGOC_LOG_DOMAIN
#define MONGOC_LOG_DOMAIN "cursor-transform"


typedef struct {
   mongoc_cursor_transform_filter_t filter;
   mongoc_cursor_transform_mutate_t mutate;
   mongoc_cursor_transform_dtor_t dtor;
   void *ctx;
   bson_t tmp;
} mongoc_cursor_transform_t;


static void *
_mongoc_cursor_transform_new (mongoc_cursor_transform_filter_t filter,
                              mongoc_cursor_transform_mutate_t mutate,
                              mongoc_cursor_transform_dtor_t dtor,
                              void *ctx)
{
   mongoc_cursor_transform_t *transform;

   ENTRY;

   transform = (mongoc_cursor_transform_t *) bson_malloc0 (sizeof *transform);

   transform->filter = filter;
   transform->mutate = mutate;
   transform->dtor = dtor;
   transform->ctx = ctx;
   bson_init (&transform->tmp);

   RETURN (transform);
}


static void
_mongoc_cursor_transform_destroy (mongoc_cursor_t *cursor)
{
   mongoc_cursor_transform_t *transform;

   ENTRY;

   transform = (mongoc_cursor_transform_t *) cursor->iface_data;

   if (transform->dtor) {
      transform->dtor (transform->ctx);
   }

   bson_destroy (&transform->tmp);

   bson_free (cursor->iface_data);
   _mongoc_cursor_destroy (cursor);

   EXIT;
}


static bool
_mongoc_cursor_transform_next (mongoc_cursor_t *cursor, const bson_t **bson)
{
   mongoc_cursor_transform_t *transform;

   ENTRY;

   transform = (mongoc_cursor_transform_t *) cursor->iface_data;

   for (;;) {
      if (!_mongoc_cursor_next (cursor, bson)) {
         RETURN (false);
      }

      switch (transform->filter (*bson, transform->ctx)) {
      case MONGO_CURSOR_TRANSFORM_DROP:
         break;
      case MONGO_CURSOR_TRANSFORM_PASS:
         RETURN (true);
         break;
      case MONGO_CURSOR_TRANSFORM_MUTATE:
         bson_reinit (&transform->tmp);

         transform->mutate (*bson, &transform->tmp, transform->ctx);
         *bson = &transform->tmp;
         RETURN (true);

         break;
      default:
         abort ();
         break;
      }
   }
}


static mongoc_cursor_t *
_mongoc_cursor_transform_clone (const mongoc_cursor_t *cursor)
{
   mongoc_cursor_transform_t *transform;
   mongoc_cursor_t *clone_;

   ENTRY;

   transform = (mongoc_cursor_transform_t *) cursor->iface_data;

   clone_ = _mongoc_cursor_clone (cursor);
   _mongoc_cursor_transform_init (clone_,
                                  transform->filter,
                                  transform->mutate,
                                  transform->dtor,
                                  transform->ctx);

   RETURN (clone_);
}


static mongoc_cursor_interface_t gMongocCursorArray = {
   _mongoc_cursor_transform_clone,
   _mongoc_cursor_transform_destroy,
   NULL,
   _mongoc_cursor_transform_next,
};


void
_mongoc_cursor_transform_init (mongoc_cursor_t *cursor,
                               mongoc_cursor_transform_filter_t filter,
                               mongoc_cursor_transform_mutate_t mutate,
                               mongoc_cursor_transform_dtor_t dtor,
                               void *ctx)
{
   ENTRY;

   cursor->iface_data =
      _mongoc_cursor_transform_new (filter, mutate, dtor, ctx);

   memcpy (
      &cursor->iface, &gMongocCursorArray, sizeof (mongoc_cursor_interface_t));

   EXIT;
}
