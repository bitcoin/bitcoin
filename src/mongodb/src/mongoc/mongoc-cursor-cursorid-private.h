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

#ifndef MONGOC_CURSOR_CURSORID_PRIVATE_H
#define MONGOC_CURSOR_CURSORID_PRIVATE_H

#if !defined(MONGOC_COMPILATION)
#error "Only <mongoc.h> can be included directly."
#endif

#include <bson.h>

#include "mongoc-cursor-private.h"


BSON_BEGIN_DECLS


typedef struct {
   bson_t array;
   bool in_batch;
   bool in_reader;
   bson_iter_t batch_iter;
   bson_t current_doc;
} mongoc_cursor_cursorid_t;


bool
_mongoc_cursor_cursorid_start_batch (mongoc_cursor_t *cursor);
bool
_mongoc_cursor_cursorid_prime (mongoc_cursor_t *cursor);
bool
_mongoc_cursor_cursorid_next (mongoc_cursor_t *cursor, const bson_t **bson);
void
_mongoc_cursor_cursorid_init (mongoc_cursor_t *cursor, const bson_t *command);
bool
_mongoc_cursor_prepare_getmore_command (mongoc_cursor_t *cursor,
                                        bson_t *command);
void
_mongoc_cursor_cursorid_init_with_reply (mongoc_cursor_t *cursor,
                                         bson_t *reply,
                                         uint32_t server_id);

BSON_END_DECLS


#endif /* MONGOC_CURSOR_CURSORID_PRIVATE_H */
