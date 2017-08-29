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

#ifndef MONGOC_GRIDFS_FILE_PRIVATE_H
#define MONGOC_GRIDFS_FILE_PRIVATE_H

#if !defined(MONGOC_COMPILATION)
#error "Only <mongoc.h> can be included directly."
#endif

#include <bson.h>

#include "mongoc-gridfs.h"
#include "mongoc-gridfs-file.h"
#include "mongoc-gridfs-file-page.h"
#include "mongoc-cursor.h"


BSON_BEGIN_DECLS


struct _mongoc_gridfs_file_t {
   mongoc_gridfs_t *gridfs;
   bson_t bson;
   mongoc_gridfs_file_page_t *page;
   uint64_t pos;
   int32_t n;
   bson_error_t error;
   mongoc_cursor_t *cursor;
   uint32_t cursor_range[2]; /* current chunk, # of chunks */
   bool is_dirty;

   bson_value_t files_id;
   int64_t length;
   int32_t chunk_size;
   int64_t upload_date;

   char *md5;
   char *filename;
   char *content_type;
   bson_t aliases;
   bson_t metadata;
   const char *bson_md5;
   const char *bson_filename;
   const char *bson_content_type;
   bson_t bson_aliases;
   bson_t bson_metadata;
};


mongoc_gridfs_file_t *
_mongoc_gridfs_file_new_from_bson (mongoc_gridfs_t *gridfs, const bson_t *data);
mongoc_gridfs_file_t *
_mongoc_gridfs_file_new (mongoc_gridfs_t *gridfs,
                         mongoc_gridfs_file_opt_t *opt);


BSON_END_DECLS


#endif /* MONGOC_GRIDFS_FILE_PRIVATE_H */
