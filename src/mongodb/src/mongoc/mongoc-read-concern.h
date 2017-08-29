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

#ifndef MONGOC_READ_CONCERN_H
#define MONGOC_READ_CONCERN_H

#if !defined(MONGOC_INSIDE) && !defined(MONGOC_COMPILATION)
#error "Only <mongoc.h> can be included directly."
#endif

#include <bson.h>

#include "mongoc-macros.h"

BSON_BEGIN_DECLS


#define MONGOC_READ_CONCERN_LEVEL_LOCAL "local"
#define MONGOC_READ_CONCERN_LEVEL_MAJORITY "majority"
#define MONGOC_READ_CONCERN_LEVEL_LINEARIZABLE "linearizable"

typedef struct _mongoc_read_concern_t mongoc_read_concern_t;


MONGOC_EXPORT (mongoc_read_concern_t *)
mongoc_read_concern_new (void);
MONGOC_EXPORT (mongoc_read_concern_t *)
mongoc_read_concern_copy (const mongoc_read_concern_t *read_concern);
MONGOC_EXPORT (void)
mongoc_read_concern_destroy (mongoc_read_concern_t *read_concern);
MONGOC_EXPORT (const char *)
mongoc_read_concern_get_level (const mongoc_read_concern_t *read_concern);
MONGOC_EXPORT (bool)
mongoc_read_concern_set_level (mongoc_read_concern_t *read_concern,
                               const char *level);
MONGOC_EXPORT (bool)
mongoc_read_concern_append (mongoc_read_concern_t *read_concern, bson_t *doc);
MONGOC_EXPORT (bool)
mongoc_read_concern_is_default (const mongoc_read_concern_t *read_concern);

BSON_END_DECLS


#endif /* MONGOC_READ_CONCERN_H */
