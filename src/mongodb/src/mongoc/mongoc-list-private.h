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

#ifndef MONGOC_LIST_H
#define MONGOC_LIST_H

#if !defined(MONGOC_COMPILATION)
#error "Only <mongoc.h> can be included directly."
#endif

#include <bson.h>


BSON_BEGIN_DECLS


typedef struct _mongoc_list_t mongoc_list_t;


struct _mongoc_list_t {
   mongoc_list_t *next;
   void *data;
};


mongoc_list_t *
_mongoc_list_append (mongoc_list_t *list, void *data);
mongoc_list_t *
_mongoc_list_prepend (mongoc_list_t *list, void *data);
mongoc_list_t *
_mongoc_list_remove (mongoc_list_t *list, void *data);
void
_mongoc_list_foreach (mongoc_list_t *list,
                      void (*func) (void *data, void *user_data),
                      void *user_data);
void
_mongoc_list_destroy (mongoc_list_t *list);


BSON_END_DECLS


#endif /* MONGOC_LIST_H */
