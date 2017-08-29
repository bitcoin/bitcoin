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

#ifndef MONGOC_GRIDFS_FILE_LIST_PRIVATE_H
#define MONGOC_GRIDFS_FILE_LIST_PRIVATE_H

#if !defined(MONGOC_COMPILATION)
#error "Only <mongoc.h> can be included directly."
#endif

#include <bson.h>

#include "mongoc-gridfs.h"
#include "mongoc-gridfs-file.h"
#include "mongoc-cursor.h"


BSON_BEGIN_DECLS


struct _mongoc_gridfs_file_list_t {
   mongoc_gridfs_t *gridfs;
   mongoc_cursor_t *cursor;
   bson_error_t error;
};


mongoc_gridfs_file_list_t *
_mongoc_gridfs_file_list_new (mongoc_gridfs_t *gridfs,
                              const bson_t *query,
                              uint32_t limit);
mongoc_gridfs_file_list_t *
_mongoc_gridfs_file_list_new_with_opts (mongoc_gridfs_t *gridfs,
                                        const bson_t *filter,
                                        const bson_t *opts);


BSON_END_DECLS


#endif /* MONGOC_GRIDFS_FILE_LIST_PRIVATE_H */
