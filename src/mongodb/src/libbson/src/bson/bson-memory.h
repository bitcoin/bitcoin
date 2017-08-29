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


#ifndef BSON_MEMORY_H
#define BSON_MEMORY_H


#if !defined(BSON_INSIDE) && !defined(BSON_COMPILATION)
#error "Only <bson.h> can be included directly."
#endif


#include "bson-macros.h"
#include "bson-types.h"


BSON_BEGIN_DECLS


typedef void *(*bson_realloc_func) (void *mem, size_t num_bytes, void *ctx);


typedef struct _bson_mem_vtable_t {
   void *(*malloc) (size_t num_bytes);
   void *(*calloc) (size_t n_members, size_t num_bytes);
   void *(*realloc) (void *mem, size_t num_bytes);
   void (*free) (void *mem);
   void *padding[4];
} bson_mem_vtable_t;


BSON_EXPORT (void)
bson_mem_set_vtable (const bson_mem_vtable_t *vtable);
BSON_EXPORT (void)
bson_mem_restore_vtable (void);
BSON_EXPORT (void *)
bson_malloc (size_t num_bytes);
BSON_EXPORT (void *)
bson_malloc0 (size_t num_bytes);
BSON_EXPORT (void *)
bson_realloc (void *mem, size_t num_bytes);
BSON_EXPORT (void *)
bson_realloc_ctx (void *mem, size_t num_bytes, void *ctx);
BSON_EXPORT (void)
bson_free (void *mem);
BSON_EXPORT (void)
bson_zero_free (void *mem, size_t size);


BSON_END_DECLS


#endif /* BSON_MEMORY_H */
