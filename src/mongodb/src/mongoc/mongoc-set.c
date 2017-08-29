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


#include <bson.h>

#include "mongoc-set-private.h"

#undef MONGOC_LOG_DOMAIN
#define MONGOC_LOG_DOMAIN "set"

mongoc_set_t *
mongoc_set_new (size_t nitems, mongoc_set_item_dtor dtor, void *dtor_ctx)
{
   mongoc_set_t *set = (mongoc_set_t *) bson_malloc (sizeof (*set));

   set->items_allocated = nitems;
   set->items = (mongoc_set_item_t *) bson_malloc (sizeof (*set->items) *
                                                   set->items_allocated);
   set->items_len = 0;

   set->dtor = dtor;
   set->dtor_ctx = dtor_ctx;

   return set;
}

static int
mongoc_set_id_cmp (const void *a_, const void *b_)
{
   mongoc_set_item_t *a = (mongoc_set_item_t *) a_;
   mongoc_set_item_t *b = (mongoc_set_item_t *) b_;

   if (a->id == b->id) {
      return 0;
   }

   return a->id < b->id ? -1 : 1;
}

void
mongoc_set_add (mongoc_set_t *set, uint32_t id, void *item)
{
   if (set->items_len >= set->items_allocated) {
      set->items_allocated *= 2;
      set->items = (mongoc_set_item_t *) bson_realloc (
         set->items, sizeof (*set->items) * set->items_allocated);
   }

   set->items[set->items_len].id = id;
   set->items[set->items_len].item = item;

   set->items_len++;

   if (set->items_len > 1 && set->items[set->items_len - 2].id > id) {
      qsort (
         set->items, set->items_len, sizeof (*set->items), mongoc_set_id_cmp);
   }
}

void
mongoc_set_rm (mongoc_set_t *set, uint32_t id)
{
   mongoc_set_item_t *ptr;
   mongoc_set_item_t key;
   int i;

   key.id = id;

   ptr = (mongoc_set_item_t *) bsearch (
      &key, set->items, set->items_len, sizeof (key), mongoc_set_id_cmp);

   if (ptr) {
      set->dtor (ptr->item, set->dtor_ctx);

      i = ptr - set->items;

      if (i != set->items_len - 1) {
         memmove (set->items + i,
                  set->items + i + 1,
                  (set->items_len - (i + 1)) * sizeof (key));
      }

      set->items_len--;
   }
}

void *
mongoc_set_get (mongoc_set_t *set, uint32_t id)
{
   mongoc_set_item_t *ptr;
   mongoc_set_item_t key;

   key.id = id;

   ptr = (mongoc_set_item_t *) bsearch (
      &key, set->items, set->items_len, sizeof (key), mongoc_set_id_cmp);

   return ptr ? ptr->item : NULL;
}

void *
mongoc_set_get_item (mongoc_set_t *set, int idx)
{
   BSON_ASSERT (set);
   BSON_ASSERT (idx < set->items_len);

   return set->items[idx].item;
}


void *
mongoc_set_get_item_and_id (mongoc_set_t *set, int idx, uint32_t *id /* OUT */)
{
   BSON_ASSERT (set);
   BSON_ASSERT (id);
   BSON_ASSERT (idx < set->items_len);

   *id = set->items[idx].id;

   return set->items[idx].item;
}


void
mongoc_set_destroy (mongoc_set_t *set)
{
   int i;

   for (i = 0; i < set->items_len; i++) {
      set->dtor (set->items[i].item, set->dtor_ctx);
   }

   bson_free (set->items);
   bson_free (set);
}

void
mongoc_set_for_each (mongoc_set_t *set, mongoc_set_for_each_cb_t cb, void *ctx)
{
   size_t i;
   mongoc_set_item_t *old_set;
   size_t items_len;

   items_len = set->items_len;

   /* prevent undefined behavior of memcpy(NULL) */
   if (items_len == 0) {
      return;
   }

   old_set = (mongoc_set_item_t *) bson_malloc (sizeof (*old_set) * items_len);
   memcpy (old_set, set->items, sizeof (*old_set) * items_len);

   for (i = 0; i < items_len; i++) {
      if (!cb (old_set[i].item, ctx)) {
         break;
      }
   }

   bson_free (old_set);
}


static mongoc_set_item_t *
_mongoc_set_find (mongoc_set_t *set, mongoc_set_for_each_cb_t cb, void *ctx)
{
   size_t i;
   size_t items_len;
   mongoc_set_item_t *item;

   items_len = set->items_len;

   for (i = 0; i < items_len; i++) {
      item = &set->items[i];
      if (cb (item->item, ctx)) {
         return item;
      }
   }

   return NULL;
}


void *
mongoc_set_find_item (mongoc_set_t *set, mongoc_set_for_each_cb_t cb, void *ctx)
{
   mongoc_set_item_t *item;

   if ((item = _mongoc_set_find (set, cb, ctx))) {
      return item->item;
   }

   return NULL;
}


uint32_t
mongoc_set_find_id (mongoc_set_t *set, mongoc_set_for_each_cb_t cb, void *ctx)
{
   mongoc_set_item_t *item;

   if ((item = _mongoc_set_find (set, cb, ctx))) {
      return item->id;
   }

   return 0;
}
