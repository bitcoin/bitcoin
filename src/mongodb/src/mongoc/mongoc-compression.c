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


#include "mongoc-config.h"

#include "mongoc-compression-private.h"
#include "mongoc-trace-private.h"
#include "mongoc-util-private.h"

#ifdef MONGOC_ENABLE_COMPRESSION
#ifdef MONGOC_ENABLE_COMPRESSION_ZLIB
#include <zlib.h>
#endif
#ifdef MONGOC_ENABLE_COMPRESSION_SNAPPY
#include <snappy-c.h>
#endif
#endif

size_t
mongoc_compressor_max_compressed_length (int32_t compressor_id, size_t size)
{
   switch (compressor_id) {
#ifdef MONGOC_ENABLE_COMPRESSION_SNAPPY
   case MONGOC_COMPRESSOR_SNAPPY_ID:
      return snappy_max_compressed_length (size);
      break;
#endif

#ifdef MONGOC_ENABLE_COMPRESSION_ZLIB
   case MONGOC_COMPRESSOR_ZLIB_ID:
      return compressBound (size);
      break;
#endif

   case MONGOC_COMPRESSOR_NOOP_ID:
      return size;
      break;
   default:
      return 0;
   }
}

bool
mongoc_compressor_supported (const char *compressor)
{
#ifdef MONGOC_ENABLE_COMPRESSION_SNAPPY
   if (!strcasecmp (compressor, MONGOC_COMPRESSOR_SNAPPY_STR)) {
      return true;
   }
#endif

#ifdef MONGOC_ENABLE_COMPRESSION_ZLIB
   if (!strcasecmp (compressor, MONGOC_COMPRESSOR_ZLIB_STR)) {
      return true;
   }
#endif

   if (!strcasecmp (compressor, MONGOC_COMPRESSOR_NOOP_STR)) {
      return true;
   }

   return false;
}

const char *
mongoc_compressor_id_to_name (int32_t compressor_id)
{
   switch (compressor_id) {
   case MONGOC_COMPRESSOR_SNAPPY_ID:
      return MONGOC_COMPRESSOR_SNAPPY_STR;

   case MONGOC_COMPRESSOR_ZLIB_ID:
      return MONGOC_COMPRESSOR_ZLIB_STR;

   case MONGOC_COMPRESSOR_NOOP_ID:
      return MONGOC_COMPRESSOR_NOOP_STR;

   default:
      return "unknown";
   }
}

int
mongoc_compressor_name_to_id (const char *compressor)
{
#ifdef MONGOC_ENABLE_COMPRESSION_SNAPPY
   if (strcasecmp (MONGOC_COMPRESSOR_SNAPPY_STR, compressor) == 0) {
      return MONGOC_COMPRESSOR_SNAPPY_ID;
   }
#endif

#ifdef MONGOC_ENABLE_COMPRESSION_ZLIB
   if (strcasecmp (MONGOC_COMPRESSOR_ZLIB_STR, compressor) == 0) {
      return MONGOC_COMPRESSOR_ZLIB_ID;
   }
#endif

   if (strcasecmp (MONGOC_COMPRESSOR_NOOP_STR, compressor) == 0) {
      return MONGOC_COMPRESSOR_NOOP_ID;
   }

   return -1;
}

bool
mongoc_uncompress (int32_t compressor_id,
                   const uint8_t *compressed,
                   size_t compressed_len,
                   uint8_t *uncompressed,
                   size_t *uncompressed_size)
{
   switch (compressor_id) {
   case MONGOC_COMPRESSOR_SNAPPY_ID: {
#ifdef MONGOC_ENABLE_COMPRESSION_SNAPPY
      snappy_status status;
      status = snappy_uncompress ((const char *) compressed,
                                  compressed_len,
                                  (char *) uncompressed,
                                  uncompressed_size);

      return status == SNAPPY_OK;
#else
      MONGOC_WARNING ("Received snappy compressed opcode, but snappy "
                      "compression is not compiled in");
      return false;
#endif
      break;
   }

   case MONGOC_COMPRESSOR_ZLIB_ID: {
#ifdef MONGOC_ENABLE_COMPRESSION_ZLIB
      int ok;

      ok = uncompress (uncompressed,
                       (unsigned long *) uncompressed_size,
                       compressed,
                       compressed_len);

      return ok == Z_OK;
#else
      MONGOC_WARNING ("Received zlib compressed opcode, but zlib "
                      "compression is not compiled in");
      return false;
#endif
      break;
   }

   default:
      MONGOC_WARNING ("Unknown compressor ID %d", compressor_id);
   }

   return false;
}

bool
mongoc_compress (int32_t compressor_id,
                 int32_t compression_level,
                 char *uncompressed,
                 size_t uncompressed_len,
                 char *compressed,
                 size_t *compressed_len)
{
   switch (compressor_id) {
   case MONGOC_COMPRESSOR_SNAPPY_ID:
#ifdef MONGOC_ENABLE_COMPRESSION_SNAPPY
      /* No compression_level option for snappy */
      return snappy_compress (
                uncompressed, uncompressed_len, compressed, compressed_len) ==
             SNAPPY_OK;
      break;
#else
      MONGOC_ERROR ("Client attempting to use compress with snappy, but snappy "
                    "compression is not compiled in");
      return false;
#endif

   case MONGOC_COMPRESSOR_ZLIB_ID:
#ifdef MONGOC_ENABLE_COMPRESSION_ZLIB
      return compress2 ((unsigned char *) compressed,
                        (unsigned long *) compressed_len,
                        (unsigned char *) uncompressed,
                        uncompressed_len,
                        compression_level) == Z_OK;
      break;
#else
      MONGOC_ERROR ("Client attempting to use compress with zlib, but zlib "
                    "compression is not compiled in");
      return false;
#endif

   default:
      return false;
   }
}
