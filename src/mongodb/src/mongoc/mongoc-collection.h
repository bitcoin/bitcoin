/*
 * Copyright 2013-2014 MongoDB, Inc.
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

#ifndef MONGOC_COLLECTION_H
#define MONGOC_COLLECTION_H

#if !defined(MONGOC_INSIDE) && !defined(MONGOC_COMPILATION)
#error "Only <mongoc.h> can be included directly."
#endif

#include <bson.h>

#include "mongoc-macros.h"
#include "mongoc-bulk-operation.h"
#include "mongoc-flags.h"
#include "mongoc-cursor.h"
#include "mongoc-index.h"
#include "mongoc-read-prefs.h"
#include "mongoc-read-concern.h"
#include "mongoc-write-concern.h"
#include "mongoc-find-and-modify.h"


BSON_BEGIN_DECLS


typedef struct _mongoc_collection_t mongoc_collection_t;

MONGOC_EXPORT (mongoc_cursor_t *)
mongoc_collection_aggregate (mongoc_collection_t *collection,
                             mongoc_query_flags_t flags,
                             const bson_t *pipeline,
                             const bson_t *opts,
                             const mongoc_read_prefs_t *read_prefs)
   BSON_GNUC_WARN_UNUSED_RESULT;
MONGOC_EXPORT (void)
mongoc_collection_destroy (mongoc_collection_t *collection);
MONGOC_EXPORT (mongoc_collection_t *)
mongoc_collection_copy (mongoc_collection_t *collection);
MONGOC_EXPORT (mongoc_cursor_t *)
mongoc_collection_command (mongoc_collection_t *collection,
                           mongoc_query_flags_t flags,
                           uint32_t skip,
                           uint32_t limit,
                           uint32_t batch_size,
                           const bson_t *command,
                           const bson_t *fields,
                           const mongoc_read_prefs_t *read_prefs)
   BSON_GNUC_WARN_UNUSED_RESULT;
MONGOC_EXPORT (bool)
mongoc_collection_read_command_with_opts (mongoc_collection_t *collection,
                                          const bson_t *command,
                                          const mongoc_read_prefs_t *read_prefs,
                                          const bson_t *opts,
                                          bson_t *reply,
                                          bson_error_t *error);
MONGOC_EXPORT (bool)
mongoc_collection_write_command_with_opts (mongoc_collection_t *collection,
                                           const bson_t *command,
                                           const bson_t *opts,
                                           bson_t *reply,
                                           bson_error_t *error);
MONGOC_EXPORT (bool)
mongoc_collection_read_write_command_with_opts (
   mongoc_collection_t *collection,
   const bson_t *command,
   const mongoc_read_prefs_t *read_prefs /* IGNORED */,
   const bson_t *opts,
   bson_t *reply,
   bson_error_t *error);
MONGOC_EXPORT (bool)
mongoc_collection_command_simple (mongoc_collection_t *collection,
                                  const bson_t *command,
                                  const mongoc_read_prefs_t *read_prefs,
                                  bson_t *reply,
                                  bson_error_t *error);
MONGOC_EXPORT (int64_t)
mongoc_collection_count (mongoc_collection_t *collection,
                         mongoc_query_flags_t flags,
                         const bson_t *query,
                         int64_t skip,
                         int64_t limit,
                         const mongoc_read_prefs_t *read_prefs,
                         bson_error_t *error);
MONGOC_EXPORT (int64_t)
mongoc_collection_count_with_opts (mongoc_collection_t *collection,
                                   mongoc_query_flags_t flags,
                                   const bson_t *query,
                                   int64_t skip,
                                   int64_t limit,
                                   const bson_t *opts,
                                   const mongoc_read_prefs_t *read_prefs,
                                   bson_error_t *error);
MONGOC_EXPORT (bool)
mongoc_collection_drop (mongoc_collection_t *collection, bson_error_t *error);
MONGOC_EXPORT (bool)
mongoc_collection_drop_with_opts (mongoc_collection_t *collection,
                                  const bson_t *opts,
                                  bson_error_t *error);
MONGOC_EXPORT (bool)
mongoc_collection_drop_index (mongoc_collection_t *collection,
                              const char *index_name,
                              bson_error_t *error);
MONGOC_EXPORT (bool)
mongoc_collection_drop_index_with_opts (mongoc_collection_t *collection,
                                        const char *index_name,
                                        const bson_t *opts,
                                        bson_error_t *error);
MONGOC_EXPORT (bool)
mongoc_collection_create_index (mongoc_collection_t *collection,
                                const bson_t *keys,
                                const mongoc_index_opt_t *opt,
                                bson_error_t *error) BSON_GNUC_DEPRECATED;
MONGOC_EXPORT (bool)
mongoc_collection_create_index_with_opts (mongoc_collection_t *collection,
                                          const bson_t *keys,
                                          const mongoc_index_opt_t *opt,
                                          const bson_t *opts,
                                          bson_t *reply,
                                          bson_error_t *error)
   BSON_GNUC_DEPRECATED;
MONGOC_EXPORT (bool)
mongoc_collection_ensure_index (mongoc_collection_t *collection,
                                const bson_t *keys,
                                const mongoc_index_opt_t *opt,
                                bson_error_t *error)
   BSON_GNUC_DEPRECATED;
MONGOC_EXPORT (mongoc_cursor_t *)
mongoc_collection_find_indexes (mongoc_collection_t *collection,
                                bson_error_t *error);
MONGOC_EXPORT (mongoc_cursor_t *)
mongoc_collection_find (mongoc_collection_t *collection,
                        mongoc_query_flags_t flags,
                        uint32_t skip,
                        uint32_t limit,
                        uint32_t batch_size,
                        const bson_t *query,
                        const bson_t *fields,
                        const mongoc_read_prefs_t *read_prefs)
   BSON_GNUC_DEPRECATED_FOR (mongoc_collection_find_with_opts)
      BSON_GNUC_WARN_UNUSED_RESULT;
MONGOC_EXPORT (mongoc_cursor_t *)
mongoc_collection_find_with_opts (mongoc_collection_t *collection,
                                  const bson_t *filter,
                                  const bson_t *opts,
                                  const mongoc_read_prefs_t *read_prefs)
   BSON_GNUC_WARN_UNUSED_RESULT;
MONGOC_EXPORT (bool)
mongoc_collection_insert (mongoc_collection_t *collection,
                          mongoc_insert_flags_t flags,
                          const bson_t *document,
                          const mongoc_write_concern_t *write_concern,
                          bson_error_t *error);
MONGOC_EXPORT (bool)
mongoc_collection_insert_bulk (mongoc_collection_t *collection,
                               mongoc_insert_flags_t flags,
                               const bson_t **documents,
                               uint32_t n_documents,
                               const mongoc_write_concern_t *write_concern,
                               bson_error_t *error)
   BSON_GNUC_DEPRECATED_FOR (mongoc_collection_create_bulk_operation);
MONGOC_EXPORT (bool)
mongoc_collection_update (mongoc_collection_t *collection,
                          mongoc_update_flags_t flags,
                          const bson_t *selector,
                          const bson_t *update,
                          const mongoc_write_concern_t *write_concern,
                          bson_error_t *error);
MONGOC_EXPORT (bool)
mongoc_collection_delete (mongoc_collection_t *collection,
                          mongoc_delete_flags_t flags,
                          const bson_t *selector,
                          const mongoc_write_concern_t *write_concern,
                          bson_error_t *error)
   BSON_GNUC_DEPRECATED_FOR (mongoc_collection_remove);
MONGOC_EXPORT (bool)
mongoc_collection_save (mongoc_collection_t *collection,
                        const bson_t *document,
                        const mongoc_write_concern_t *write_concern,
                        bson_error_t *error)
   BSON_GNUC_DEPRECATED_FOR (mongoc_collection_insert or
                             mongoc_collection_update);
MONGOC_EXPORT (bool)
mongoc_collection_remove (mongoc_collection_t *collection,
                          mongoc_remove_flags_t flags,
                          const bson_t *selector,
                          const mongoc_write_concern_t *write_concern,
                          bson_error_t *error);
MONGOC_EXPORT (bool)
mongoc_collection_rename (mongoc_collection_t *collection,
                          const char *new_db,
                          const char *new_name,
                          bool drop_target_before_rename,
                          bson_error_t *error);
MONGOC_EXPORT (bool)
mongoc_collection_rename_with_opts (mongoc_collection_t *collection,
                                    const char *new_db,
                                    const char *new_name,
                                    bool drop_target_before_rename,
                                    const bson_t *opts,
                                    bson_error_t *error);
MONGOC_EXPORT (bool)
mongoc_collection_find_and_modify_with_opts (
   mongoc_collection_t *collection,
   const bson_t *query,
   const mongoc_find_and_modify_opts_t *opts,
   bson_t *reply,
   bson_error_t *error);
MONGOC_EXPORT (bool)
mongoc_collection_find_and_modify (mongoc_collection_t *collection,
                                   const bson_t *query,
                                   const bson_t *sort,
                                   const bson_t *update,
                                   const bson_t *fields,
                                   bool _remove,
                                   bool upsert,
                                   bool _new,
                                   bson_t *reply,
                                   bson_error_t *error);
MONGOC_EXPORT (bool)
mongoc_collection_stats (mongoc_collection_t *collection,
                         const bson_t *options,
                         bson_t *reply,
                         bson_error_t *error);
MONGOC_EXPORT (mongoc_bulk_operation_t *)
mongoc_collection_create_bulk_operation (
   mongoc_collection_t *collection,
   bool ordered,
   const mongoc_write_concern_t *write_concern) BSON_GNUC_WARN_UNUSED_RESULT;
MONGOC_EXPORT (const mongoc_read_prefs_t *)
mongoc_collection_get_read_prefs (const mongoc_collection_t *collection);
MONGOC_EXPORT (void)
mongoc_collection_set_read_prefs (mongoc_collection_t *collection,
                                  const mongoc_read_prefs_t *read_prefs);
MONGOC_EXPORT (const mongoc_read_concern_t *)
mongoc_collection_get_read_concern (const mongoc_collection_t *collection);
MONGOC_EXPORT (void)
mongoc_collection_set_read_concern (mongoc_collection_t *collection,
                                    const mongoc_read_concern_t *read_concern);
MONGOC_EXPORT (const mongoc_write_concern_t *)
mongoc_collection_get_write_concern (const mongoc_collection_t *collection);
MONGOC_EXPORT (void)
mongoc_collection_set_write_concern (
   mongoc_collection_t *collection,
   const mongoc_write_concern_t *write_concern);
MONGOC_EXPORT (const char *)
mongoc_collection_get_name (mongoc_collection_t *collection);
MONGOC_EXPORT (const bson_t *)
mongoc_collection_get_last_error (const mongoc_collection_t *collection);
MONGOC_EXPORT (char *)
mongoc_collection_keys_to_index_string (const bson_t *keys);
MONGOC_EXPORT (bool)
mongoc_collection_validate (mongoc_collection_t *collection,
                            const bson_t *options,
                            bson_t *reply,
                            bson_error_t *error);


BSON_END_DECLS


#endif /* MONGOC_COLLECTION_H */
