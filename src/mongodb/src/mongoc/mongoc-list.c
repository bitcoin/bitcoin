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


#include "mongoc-list-private.h"


/**
 * mongoc_list_append:
 * @list: A list to append to, or NULL.
 * @data: Data to append to @list.
 *
 * Appends a new link onto the linked list.
 *
 * Returns: @list or a new list if @list is NULL.
 */
mongoc_list_t *
_mongoc_list_append (mongoc_list_t *list, void *data)
{
   mongoc_list_t *item;
   mongoc_list_t *iter;

   item = (mongoc_list_t *) bson_malloc0 (sizeof *item);
   item->data = (void *) data;
   if (!list) {
      return item;
   }

   for (iter = list; iter->next; iter = iter->next) {
   }
   iter->next = item;

   return list;
}


/**
 * mongoc_list_prepend:
 * @list: A mongoc_list_t or NULL.
 * @data: data to prepend to the list.
 *
 * Prepends to @list a new link containing @data.
 *
 * Returns: A new link containing data with @list following.
 */
mongoc_list_t *
_mongoc_list_prepend (mongoc_list_t *list, void *data)
{
   mongoc_list_t *item;

   item = (mongoc_list_t *) bson_malloc0 (sizeof *item);
   item->data = (void *) data;
   item->next = list;

   return item;
}


/**
 * mongoc_list_remove:
 * @list: A mongoc_list_t.
 * @data: Data to remove from @list.
 *
 * Removes the link containing @data from @list.
 *
 * Returns: @list with the link containing @data removed.
 */
mongoc_list_t *
_mongoc_list_remove (mongoc_list_t *list, void *data)
{
   mongoc_list_t *iter;
   mongoc_list_t *prev = NULL;
   mongoc_list_t *ret = list;

   BSON_ASSERT (list);

   for (iter = list; iter; iter = iter->next) {
      if (iter->data == data) {
         if (iter != list) {
            prev->next = iter->next;
         } else {
            ret = iter->next;
         }
         bson_free (iter);
         break;
      }
      prev = iter;
   }

   return ret;
}


/**
 * mongoc_list_foreach:
 * @list: A mongoc_list_t or NULL.
 * @func: A func to call for each link in @list.
 * @user_data: User data for @func.
 *
 * Calls @func for each item in @list.
 */
void
_mongoc_list_foreach (mongoc_list_t *list,
                      void (*func) (void *data, void *user_data),
                      void *user_data)
{
   mongoc_list_t *iter;

   BSON_ASSERT (func);

   for (iter = list; iter; iter = iter->next) {
      func (iter->data, user_data);
   }
}


/**
 * mongoc_list_destroy:
 * @list: A mongoc_list_t.
 *
 * Destroys @list and releases any resources.
 */
void
_mongoc_list_destroy (mongoc_list_t *list)
{
   mongoc_list_t *tmp = list;

   while (list) {
      tmp = list->next;
      bson_free (list);
      list = tmp;
   }
}
