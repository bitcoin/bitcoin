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

#ifndef MONGOC_GRIDFS_FILE_PAGE_PRIVATE_H
#define MONGOC_GRIDFS_FILE_PAGE_PRIVATE_H

#if !defined(MONGOC_COMPILATION)
#error "Only <mongoc.h> can be included directly."
#endif

#include <bson.h>

#include "mongoc-gridfs-file.h"


BSON_BEGIN_DECLS


struct _mongoc_gridfs_file_page_t {
   const uint8_t *read_buf;
   uint8_t *buf;
   uint32_t len;
   uint32_t chunk_size;
   uint32_t offset;
};


mongoc_gridfs_file_page_t *
_mongoc_gridfs_file_page_new (const uint8_t *data,
                              uint32_t len,
                              uint32_t chunk_size);
void
_mongoc_gridfs_file_page_destroy (mongoc_gridfs_file_page_t *page);
bool
_mongoc_gridfs_file_page_seek (mongoc_gridfs_file_page_t *page,
                               uint32_t offset);
int32_t
_mongoc_gridfs_file_page_read (mongoc_gridfs_file_page_t *page,
                               void *dst,
                               uint32_t len);
int32_t
_mongoc_gridfs_file_page_write (mongoc_gridfs_file_page_t *page,
                                const void *src,
                                uint32_t len);
uint32_t
_mongoc_gridfs_file_page_memset0 (mongoc_gridfs_file_page_t *page,
                                  uint32_t len);
uint32_t
_mongoc_gridfs_file_page_tell (mongoc_gridfs_file_page_t *page);
const uint8_t *
_mongoc_gridfs_file_page_get_data (mongoc_gridfs_file_page_t *page);
uint32_t
_mongoc_gridfs_file_page_get_len (mongoc_gridfs_file_page_t *page);
bool
_mongoc_gridfs_file_page_is_dirty (mongoc_gridfs_file_page_t *page);


BSON_END_DECLS


#endif /* MONGOC_GRIDFS_FILE_PAGE_PRIVATE_H */
