/*
 * Copyright 2013 MongoDB Inc.
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

#ifndef MONGOC_GRIDFS_H
#define MONGOC_GRIDFS_H

#if !defined(MONGOC_INSIDE) && !defined(MONGOC_COMPILATION)
#error "Only <mongoc.h> can be included directly."
#endif

#include <bson.h>

#include "mongoc-macros.h"
#include "mongoc-stream.h"
#include "mongoc-gridfs-file.h"
#include "mongoc-collection.h"
#include "mongoc-gridfs-file-list.h"


BSON_BEGIN_DECLS


typedef struct _mongoc_gridfs_t mongoc_gridfs_t;


MONGOC_EXPORT (mongoc_gridfs_file_t *)
mongoc_gridfs_create_file_from_stream (mongoc_gridfs_t *gridfs,
                                       mongoc_stream_t *stream,
                                       mongoc_gridfs_file_opt_t *opt);
MONGOC_EXPORT (mongoc_gridfs_file_t *)
mongoc_gridfs_create_file (mongoc_gridfs_t *gridfs,
                           mongoc_gridfs_file_opt_t *opt);
MONGOC_EXPORT (mongoc_gridfs_file_list_t *)
mongoc_gridfs_find (mongoc_gridfs_t *gridfs, const bson_t *query)
   BSON_GNUC_DEPRECATED_FOR (mongoc_gridfs_find_with_opts);
MONGOC_EXPORT (mongoc_gridfs_file_t *)
mongoc_gridfs_find_one (mongoc_gridfs_t *gridfs,
                        const bson_t *query,
                        bson_error_t *error)
   BSON_GNUC_DEPRECATED_FOR (mongoc_gridfs_find_one_with_opts);
MONGOC_EXPORT (mongoc_gridfs_file_list_t *)
mongoc_gridfs_find_with_opts (mongoc_gridfs_t *gridfs,
                              const bson_t *filter,
                              const bson_t *opts) BSON_GNUC_WARN_UNUSED_RESULT;
MONGOC_EXPORT (mongoc_gridfs_file_t *)
mongoc_gridfs_find_one_with_opts (mongoc_gridfs_t *gridfs,
                                  const bson_t *filter,
                                  const bson_t *opts,
                                  bson_error_t *error)
   BSON_GNUC_WARN_UNUSED_RESULT;
MONGOC_EXPORT (mongoc_gridfs_file_t *)
mongoc_gridfs_find_one_by_filename (mongoc_gridfs_t *gridfs,
                                    const char *filename,
                                    bson_error_t *error);
MONGOC_EXPORT (bool)
mongoc_gridfs_drop (mongoc_gridfs_t *gridfs, bson_error_t *error);
MONGOC_EXPORT (void)
mongoc_gridfs_destroy (mongoc_gridfs_t *gridfs);
MONGOC_EXPORT (mongoc_collection_t *)
mongoc_gridfs_get_files (mongoc_gridfs_t *gridfs);
MONGOC_EXPORT (mongoc_collection_t *)
mongoc_gridfs_get_chunks (mongoc_gridfs_t *gridfs);
MONGOC_EXPORT (bool)
mongoc_gridfs_remove_by_filename (mongoc_gridfs_t *gridfs,
                                  const char *filename,
                                  bson_error_t *error);


BSON_END_DECLS


#endif /* MONGOC_GRIDFS_H */
