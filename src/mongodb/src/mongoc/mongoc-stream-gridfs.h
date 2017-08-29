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

#ifndef MONGOC_STREAM_GRIDFS_H
#define MONGOC_STREAM_GRIDFS_H

#if !defined(MONGOC_INSIDE) && !defined(MONGOC_COMPILATION)
#error "Only <mongoc.h> can be included directly."
#endif

#include <bson.h>

#include "mongoc-macros.h"
#include "mongoc-gridfs.h"
#include "mongoc-stream.h"


BSON_BEGIN_DECLS


MONGOC_EXPORT (mongoc_stream_t *)
mongoc_stream_gridfs_new (mongoc_gridfs_file_t *file);


BSON_END_DECLS


#endif /* MONGOC_STREAM_GRIDFS_H */
