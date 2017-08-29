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

#ifndef MONGOC_DATABASE_H
#define MONGOC_DATABASE_H

#if !defined(MONGOC_INSIDE) && !defined(MONGOC_COMPILATION)
#error "Only <mongoc.h> can be included directly."
#endif

#include <bson.h>

#include "mongoc-macros.h"
#include "mongoc-cursor.h"
#include "mongoc-flags.h"
#include "mongoc-read-prefs.h"
#include "mongoc-read-concern.h"
#include "mongoc-write-concern.h"


BSON_BEGIN_DECLS


typedef struct _mongoc_database_t mongoc_database_t;


MONGOC_EXPORT (const char *)
mongoc_database_get_name (mongoc_database_t *database);
MONGOC_EXPORT (bool)
mongoc_database_remove_user (mongoc_database_t *database,
                             const char *username,
                             bson_error_t *error);
MONGOC_EXPORT (bool)
mongoc_database_remove_all_users (mongoc_database_t *database,
                                  bson_error_t *error);
MONGOC_EXPORT (bool)
mongoc_database_add_user (mongoc_database_t *database,
                          const char *username,
                          const char *password,
                          const bson_t *roles,
                          const bson_t *custom_data,
                          bson_error_t *error);
MONGOC_EXPORT (void)
mongoc_database_destroy (mongoc_database_t *database);
MONGOC_EXPORT (mongoc_database_t *)
mongoc_database_copy (mongoc_database_t *database);
MONGOC_EXPORT (mongoc_cursor_t *)
mongoc_database_command (mongoc_database_t *database,
                         mongoc_query_flags_t flags,
                         uint32_t skip,
                         uint32_t limit,
                         uint32_t batch_size,
                         const bson_t *command,
                         const bson_t *fields,
                         const mongoc_read_prefs_t *read_prefs);
MONGOC_EXPORT (bool)
mongoc_database_read_command_with_opts (mongoc_database_t *database,
                                        const bson_t *command,
                                        const mongoc_read_prefs_t *read_prefs,
                                        const bson_t *opts,
                                        bson_t *reply,
                                        bson_error_t *error);
MONGOC_EXPORT (bool)
mongoc_database_write_command_with_opts (mongoc_database_t *database,
                                         const bson_t *command,
                                         const bson_t *opts,
                                         bson_t *reply,
                                         bson_error_t *error);
MONGOC_EXPORT (bool)
mongoc_database_read_write_command_with_opts (
   mongoc_database_t *database,
   const bson_t *command,
   const mongoc_read_prefs_t *read_prefs /* IGNORED */,
   const bson_t *opts,
   bson_t *reply,
   bson_error_t *error);
MONGOC_EXPORT (bool)
mongoc_database_command_simple (mongoc_database_t *database,
                                const bson_t *command,
                                const mongoc_read_prefs_t *read_prefs,
                                bson_t *reply,
                                bson_error_t *error);
MONGOC_EXPORT (bool)
mongoc_database_drop (mongoc_database_t *database, bson_error_t *error);
MONGOC_EXPORT (bool)
mongoc_database_drop_with_opts (mongoc_database_t *database,
                                const bson_t *opts,
                                bson_error_t *error);
MONGOC_EXPORT (bool)
mongoc_database_has_collection (mongoc_database_t *database,
                                const char *name,
                                bson_error_t *error);
MONGOC_EXPORT (mongoc_collection_t *)
mongoc_database_create_collection (mongoc_database_t *database,
                                   const char *name,
                                   const bson_t *options,
                                   bson_error_t *error);
MONGOC_EXPORT (const mongoc_read_prefs_t *)
mongoc_database_get_read_prefs (const mongoc_database_t *database);
MONGOC_EXPORT (void)
mongoc_database_set_read_prefs (mongoc_database_t *database,
                                const mongoc_read_prefs_t *read_prefs);
MONGOC_EXPORT (const mongoc_write_concern_t *)
mongoc_database_get_write_concern (const mongoc_database_t *database);
MONGOC_EXPORT (void)
mongoc_database_set_write_concern (mongoc_database_t *database,
                                   const mongoc_write_concern_t *write_concern);
MONGOC_EXPORT (const mongoc_read_concern_t *)
mongoc_database_get_read_concern (const mongoc_database_t *database);
MONGOC_EXPORT (void)
mongoc_database_set_read_concern (mongoc_database_t *database,
                                  const mongoc_read_concern_t *read_concern);
MONGOC_EXPORT (mongoc_cursor_t *)
mongoc_database_find_collections (mongoc_database_t *database,
                                  const bson_t *filter,
                                  bson_error_t *error);
MONGOC_EXPORT (char **)
mongoc_database_get_collection_names (mongoc_database_t *database,
                                      bson_error_t *error);
MONGOC_EXPORT (mongoc_collection_t *)
mongoc_database_get_collection (mongoc_database_t *database, const char *name);


BSON_END_DECLS


#endif /* MONGOC_DATABASE_H */
