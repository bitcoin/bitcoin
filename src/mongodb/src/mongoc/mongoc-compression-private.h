/*
 * Copyright 2017 MongoDB Inc.
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


#ifndef MONGOC_COMPRESSION_PRIVATE_H
#define MONGOC_COMPRESSION_PRIVATE_H

#if !defined(MONGOC_INSIDE) && !defined(MONGOC_COMPILATION)
#error "Only <mongoc.h> can be included directly."
#endif
#include <bson.h>


/* Compressor IDs */
#define MONGOC_COMPRESSOR_NOOP_ID 0
#define MONGOC_COMPRESSOR_NOOP_STR "noop"

#define MONGOC_COMPRESSOR_SNAPPY_ID 1
#define MONGOC_COMPRESSOR_SNAPPY_STR "snappy"

#define MONGOC_COMPRESSOR_ZLIB_ID 2
#define MONGOC_COMPRESSOR_ZLIB_STR "zlib"


BSON_BEGIN_DECLS


size_t
mongoc_compressor_max_compressed_length (int32_t compressor_id, size_t size);

bool
mongoc_compressor_supported (const char *compressor);

const char *
mongoc_compressor_id_to_name (int32_t compressor_id);

int
mongoc_compressor_name_to_id (const char *compressor);

bool
mongoc_uncompress (int32_t compressor_id,
                   const uint8_t *compressed,
                   size_t compressed_len,
                   uint8_t *uncompressed,
                   size_t *uncompressed_size);

bool
mongoc_compress (int32_t compressor_id,
                 int32_t compression_level,
                 char *uncompressed,
                 size_t uncompressed_len,
                 char *compressed,
                 size_t *compressed_len);

BSON_END_DECLS

#endif
