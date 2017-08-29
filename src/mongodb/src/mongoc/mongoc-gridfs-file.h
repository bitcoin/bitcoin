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

#ifndef MONGOC_GRIDFS_FILE_H
#define MONGOC_GRIDFS_FILE_H

#if !defined(MONGOC_INSIDE) && !defined(MONGOC_COMPILATION)
#error "Only <mongoc.h> can be included directly."
#endif

#include <bson.h>

#include "mongoc-macros.h"
#include "mongoc-socket.h"

BSON_BEGIN_DECLS


#define MONGOC_GRIDFS_FILE_STR_HEADER(name)                    \
   MONGOC_EXPORT (const char *)                                \
   mongoc_gridfs_file_get_##name (mongoc_gridfs_file_t *file); \
   MONGOC_EXPORT (void)                                        \
   mongoc_gridfs_file_set_##name (mongoc_gridfs_file_t *file, const char *str);


#define MONGOC_GRIDFS_FILE_BSON_HEADER(name)                   \
   MONGOC_EXPORT (const bson_t *)                              \
   mongoc_gridfs_file_get_##name (mongoc_gridfs_file_t *file); \
   MONGOC_EXPORT (void)                                        \
   mongoc_gridfs_file_set_##name (mongoc_gridfs_file_t *file,  \
                                  const bson_t *bson);


typedef struct _mongoc_gridfs_file_t mongoc_gridfs_file_t;
typedef struct _mongoc_gridfs_file_opt_t mongoc_gridfs_file_opt_t;


struct _mongoc_gridfs_file_opt_t {
   const char *md5;
   const char *filename;
   const char *content_type;
   const bson_t *aliases;
   const bson_t *metadata;
   uint32_t chunk_size;
};


MONGOC_GRIDFS_FILE_STR_HEADER (md5)
MONGOC_GRIDFS_FILE_STR_HEADER (filename)
MONGOC_GRIDFS_FILE_STR_HEADER (content_type)
MONGOC_GRIDFS_FILE_BSON_HEADER (aliases)
MONGOC_GRIDFS_FILE_BSON_HEADER (metadata)


MONGOC_EXPORT (const bson_value_t *)
mongoc_gridfs_file_get_id (mongoc_gridfs_file_t *file);

MONGOC_EXPORT (int64_t)
mongoc_gridfs_file_get_length (mongoc_gridfs_file_t *file);

MONGOC_EXPORT (int32_t)
mongoc_gridfs_file_get_chunk_size (mongoc_gridfs_file_t *file);

MONGOC_EXPORT (int64_t)
mongoc_gridfs_file_get_upload_date (mongoc_gridfs_file_t *file);

MONGOC_EXPORT (ssize_t)
mongoc_gridfs_file_writev (mongoc_gridfs_file_t *file,
                           const mongoc_iovec_t *iov,
                           size_t iovcnt,
                           uint32_t timeout_msec);
MONGOC_EXPORT (ssize_t)
mongoc_gridfs_file_readv (mongoc_gridfs_file_t *file,
                          mongoc_iovec_t *iov,
                          size_t iovcnt,
                          size_t min_bytes,
                          uint32_t timeout_msec);
MONGOC_EXPORT (int)
mongoc_gridfs_file_seek (mongoc_gridfs_file_t *file, int64_t delta, int whence);

MONGOC_EXPORT (uint64_t)
mongoc_gridfs_file_tell (mongoc_gridfs_file_t *file);

MONGOC_EXPORT (bool)
mongoc_gridfs_file_set_id (mongoc_gridfs_file_t *file,
                           const bson_value_t *id,
                           bson_error_t *error);

MONGOC_EXPORT (bool)
mongoc_gridfs_file_save (mongoc_gridfs_file_t *file);

MONGOC_EXPORT (void)
mongoc_gridfs_file_destroy (mongoc_gridfs_file_t *file);

MONGOC_EXPORT (bool)
mongoc_gridfs_file_error (mongoc_gridfs_file_t *file, bson_error_t *error);

MONGOC_EXPORT (bool)
mongoc_gridfs_file_remove (mongoc_gridfs_file_t *file, bson_error_t *error);

BSON_END_DECLS

#endif /* MONGOC_GRIDFS_FILE_H */
