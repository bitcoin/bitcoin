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


#ifndef BSON_PRIVATE_H
#define BSON_PRIVATE_H


#include "bson-macros.h"
#include "bson-memory.h"
#include "bson-types.h"


#if (__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
#define BEGIN_IGNORE_DEPRECATIONS  \
   _Pragma ("GCC diagnostic push") \
      _Pragma ("GCC diagnostic ignored \"-Wdeprecated-declarations\"")
#define END_IGNORE_DEPRECATIONS _Pragma ("GCC diagnostic pop")
#elif defined(__clang__)
#define BEGIN_IGNORE_DEPRECATIONS    \
   _Pragma ("clang diagnostic push") \
      _Pragma ("clang diagnostic ignored \"-Wdeprecated-declarations\"")
#define END_IGNORE_DEPRECATIONS _Pragma ("clang diagnostic pop")
#else
#define BEGIN_IGNORE_DEPRECATIONS
#define END_IGNORE_DEPRECATIONS
#endif


BSON_BEGIN_DECLS


typedef enum {
   BSON_FLAG_NONE = 0,
   BSON_FLAG_INLINE = (1 << 0),
   BSON_FLAG_STATIC = (1 << 1),
   BSON_FLAG_RDONLY = (1 << 2),
   BSON_FLAG_CHILD = (1 << 3),
   BSON_FLAG_IN_CHILD = (1 << 4),
   BSON_FLAG_NO_FREE = (1 << 5),
} bson_flags_t;


#define BSON_INLINE_DATA_SIZE 120

BSON_ALIGNED_BEGIN (128)
typedef struct {
   bson_flags_t flags;
   uint32_t len;
   uint8_t data[BSON_INLINE_DATA_SIZE];
} bson_impl_inline_t BSON_ALIGNED_END (128);


BSON_STATIC_ASSERT (sizeof (bson_impl_inline_t) == 128);


BSON_ALIGNED_BEGIN (128)
typedef struct {
   bson_flags_t flags;        /* flags describing the bson_t */
   uint32_t len;              /* length of bson document in bytes */
   bson_t *parent;            /* parent bson if a child */
   uint32_t depth;            /* Subdocument depth. */
   uint8_t **buf;             /* pointer to buffer pointer */
   size_t *buflen;            /* pointer to buffer length */
   size_t offset;             /* our offset inside *buf  */
   uint8_t *alloc;            /* buffer that we own. */
   size_t alloclen;           /* length of buffer that we own. */
   bson_realloc_func realloc; /* our realloc implementation */
   void *realloc_func_ctx;    /* context for our realloc func */
} bson_impl_alloc_t BSON_ALIGNED_END (128);


BSON_STATIC_ASSERT (sizeof (bson_impl_alloc_t) <= 128);


#define BSON_REGEX_OPTIONS_SORTED "ilmsux"

BSON_END_DECLS


#endif /* BSON_PRIVATE_H */
