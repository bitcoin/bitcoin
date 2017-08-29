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

#ifndef MONGOC_GRIDFS_PRIVATE_H
#define MONGOC_GRIDFS_PRIVATE_H

#if !defined(MONGOC_COMPILATION)
#error "Only <mongoc.h> can be included directly."
#endif

#include <bson.h>

#include "mongoc-read-prefs.h"
#include "mongoc-write-concern.h"
#include "mongoc-client.h"


BSON_BEGIN_DECLS


struct _mongoc_gridfs_t {
   mongoc_client_t *client;
   mongoc_collection_t *files;
   mongoc_collection_t *chunks;
};


mongoc_gridfs_t *
_mongoc_gridfs_new (mongoc_client_t *client,
                    const char *db,
                    const char *prefix,
                    bson_error_t *error);


BSON_END_DECLS


#endif /* MONGOC_GRIDFS_PRIVATE_H */
