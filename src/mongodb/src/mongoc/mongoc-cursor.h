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

#ifndef MONGOC_CURSOR_H
#define MONGOC_CURSOR_H

#if !defined(MONGOC_INSIDE) && !defined(MONGOC_COMPILATION)
#error "Only <mongoc.h> can be included directly."
#endif

#include <bson.h>

#include "mongoc-macros.h"
#include "mongoc-host-list.h"


BSON_BEGIN_DECLS

typedef struct _mongoc_cursor_t mongoc_cursor_t;


/* forward decl */
struct _mongoc_client_t;

MONGOC_EXPORT (mongoc_cursor_t *)
mongoc_cursor_clone (const mongoc_cursor_t *cursor)
   BSON_GNUC_WARN_UNUSED_RESULT;
MONGOC_EXPORT (void)
mongoc_cursor_destroy (mongoc_cursor_t *cursor);
MONGOC_EXPORT (bool)
mongoc_cursor_more (mongoc_cursor_t *cursor);
MONGOC_EXPORT (bool)
mongoc_cursor_next (mongoc_cursor_t *cursor, const bson_t **bson);
MONGOC_EXPORT (bool)
mongoc_cursor_error (mongoc_cursor_t *cursor, bson_error_t *error);
MONGOC_EXPORT (bool)
mongoc_cursor_error_document (mongoc_cursor_t *cursor,
                              bson_error_t *error,
                              const bson_t **doc);
MONGOC_EXPORT (void)
mongoc_cursor_get_host (mongoc_cursor_t *cursor, mongoc_host_list_t *host);
MONGOC_EXPORT (bool)
mongoc_cursor_is_alive (const mongoc_cursor_t *cursor);
MONGOC_EXPORT (const bson_t *)
mongoc_cursor_current (const mongoc_cursor_t *cursor);
MONGOC_EXPORT (void)
mongoc_cursor_set_batch_size (mongoc_cursor_t *cursor, uint32_t batch_size);
MONGOC_EXPORT (uint32_t)
mongoc_cursor_get_batch_size (const mongoc_cursor_t *cursor);
MONGOC_EXPORT (bool)
mongoc_cursor_set_limit (mongoc_cursor_t *cursor, int64_t limit);
MONGOC_EXPORT (int64_t)
mongoc_cursor_get_limit (const mongoc_cursor_t *cursor);
/* These names include the term "hint" for backward compatibility, should be
 * mongoc_cursor_get_server_id, mongoc_cursor_set_server_id. */
MONGOC_EXPORT (bool)
mongoc_cursor_set_hint (mongoc_cursor_t *cursor, uint32_t server_id);
MONGOC_EXPORT (uint32_t)
mongoc_cursor_get_hint (const mongoc_cursor_t *cursor);
MONGOC_EXPORT (int64_t)
mongoc_cursor_get_id (const mongoc_cursor_t *cursor);
MONGOC_EXPORT (void)
mongoc_cursor_set_max_await_time_ms (mongoc_cursor_t *cursor,
                                     uint32_t max_await_time_ms);
MONGOC_EXPORT (uint32_t)
mongoc_cursor_get_max_await_time_ms (const mongoc_cursor_t *cursor);
MONGOC_EXPORT (mongoc_cursor_t *)
mongoc_cursor_new_from_command_reply (struct _mongoc_client_t *client,
                                      bson_t *reply,
                                      uint32_t server_id)
   BSON_GNUC_WARN_UNUSED_RESULT;

BSON_END_DECLS


#endif /* MONGOC_CURSOR_H */
