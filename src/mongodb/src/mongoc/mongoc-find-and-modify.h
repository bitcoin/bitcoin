/*
 * Copyright 2015 MongoDB, Inc.
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

#ifndef MONGOC_FIND_AND_MODIFY_H
#define MONGOC_FIND_AND_MODIFY_H

#if !defined(MONGOC_INSIDE) && !defined(MONGOC_COMPILATION)
#error "Only <mongoc.h> can be included directly."
#endif

#include <bson.h>

#include "mongoc-macros.h"

BSON_BEGIN_DECLS

typedef enum {
   MONGOC_FIND_AND_MODIFY_NONE = 0,
   MONGOC_FIND_AND_MODIFY_REMOVE = 1 << 0,
   MONGOC_FIND_AND_MODIFY_UPSERT = 1 << 1,
   MONGOC_FIND_AND_MODIFY_RETURN_NEW = 1 << 2,
} mongoc_find_and_modify_flags_t;

typedef struct _mongoc_find_and_modify_opts_t mongoc_find_and_modify_opts_t;

MONGOC_EXPORT (mongoc_find_and_modify_opts_t *)
mongoc_find_and_modify_opts_new (void);

MONGOC_EXPORT (bool)
mongoc_find_and_modify_opts_set_sort (mongoc_find_and_modify_opts_t *opts,
                                      const bson_t *sort);

MONGOC_EXPORT (void)
mongoc_find_and_modify_opts_get_sort (const mongoc_find_and_modify_opts_t *opts,
                                      bson_t *sort);

MONGOC_EXPORT (bool)
mongoc_find_and_modify_opts_set_update (mongoc_find_and_modify_opts_t *opts,
                                        const bson_t *update);

MONGOC_EXPORT (void)
mongoc_find_and_modify_opts_get_update (
   const mongoc_find_and_modify_opts_t *opts, bson_t *update);

MONGOC_EXPORT (bool)
mongoc_find_and_modify_opts_set_fields (mongoc_find_and_modify_opts_t *opts,
                                        const bson_t *fields);

MONGOC_EXPORT (void)
mongoc_find_and_modify_opts_get_fields (
   const mongoc_find_and_modify_opts_t *opts, bson_t *fields);

MONGOC_EXPORT (bool)
mongoc_find_and_modify_opts_set_flags (
   mongoc_find_and_modify_opts_t *opts,
   const mongoc_find_and_modify_flags_t flags);

MONGOC_EXPORT (mongoc_find_and_modify_flags_t)
mongoc_find_and_modify_opts_get_flags (
   const mongoc_find_and_modify_opts_t *opts);

MONGOC_EXPORT (bool)
mongoc_find_and_modify_opts_set_bypass_document_validation (
   mongoc_find_and_modify_opts_t *opts, bool bypass);

MONGOC_EXPORT (bool)
mongoc_find_and_modify_opts_get_bypass_document_validation (
   const mongoc_find_and_modify_opts_t *opts);

MONGOC_EXPORT (bool)
mongoc_find_and_modify_opts_set_max_time_ms (
   mongoc_find_and_modify_opts_t *opts, uint32_t max_time_ms);

MONGOC_EXPORT (uint32_t)
mongoc_find_and_modify_opts_get_max_time_ms (
   const mongoc_find_and_modify_opts_t *opts);

MONGOC_EXPORT (bool)
mongoc_find_and_modify_opts_append (mongoc_find_and_modify_opts_t *opts,
                                    const bson_t *extra);

MONGOC_EXPORT (void)
mongoc_find_and_modify_opts_get_extra (
   const mongoc_find_and_modify_opts_t *opts, bson_t *extra);

MONGOC_EXPORT (void)
mongoc_find_and_modify_opts_destroy (mongoc_find_and_modify_opts_t *opts);

BSON_END_DECLS


#endif /* MONGOC_FIND_AND_MODIFY_H */
