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

#ifndef MONGOC_QUEUE_PRIVATE_H
#define MONGOC_QUEUE_PRIVATE_H

#if !defined(MONGOC_COMPILATION)
#error "Only <mongoc.h> can be included directly."
#endif

#include <bson.h>

#include "mongoc-list-private.h"


BSON_BEGIN_DECLS


#define MONGOC_QUEUE_INITIALIZER \
   {                             \
      NULL, NULL                 \
   }


typedef struct _mongoc_queue_t mongoc_queue_t;
typedef struct _mongoc_queue_item_t mongoc_queue_item_t;


struct _mongoc_queue_t {
   mongoc_queue_item_t *head;
   mongoc_queue_item_t *tail;
   uint32_t length;
};


struct _mongoc_queue_item_t {
   mongoc_queue_item_t *next;
   void *data;
};


void
_mongoc_queue_init (mongoc_queue_t *queue);
void *
_mongoc_queue_pop_head (mongoc_queue_t *queue);
void *
_mongoc_queue_pop_tail (mongoc_queue_t *queue);
void
_mongoc_queue_push_head (mongoc_queue_t *queue, void *data);
void
_mongoc_queue_push_tail (mongoc_queue_t *queue, void *data);
uint32_t
_mongoc_queue_get_length (const mongoc_queue_t *queue);


BSON_END_DECLS


#endif /* MONGOC_QUEUE_PRIVATE_H */
