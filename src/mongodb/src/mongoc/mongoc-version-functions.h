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


#if !defined(MONGOC_INSIDE) && !defined(MONGOC_COMPILATION)
#error "Only <mongoc.h> can be included directly."
#endif


#ifndef MONGOC_VERSION_FUNCTIONS_H
#define MONGOC_VERSION_FUNCTIONS_H

#include <bson.h> /* for "bool" */

#include "mongoc-macros.h"

BSON_BEGIN_DECLS

MONGOC_EXPORT (int)
mongoc_get_major_version (void);
MONGOC_EXPORT (int)
mongoc_get_minor_version (void);
MONGOC_EXPORT (int)
mongoc_get_micro_version (void);
MONGOC_EXPORT (const char *)
mongoc_get_version (void);
MONGOC_EXPORT (bool)
mongoc_check_version (int required_major,
                      int required_minor,
                      int required_micro);

BSON_END_DECLS

#endif /* MONGOC_VERSION_FUNCTIONS_H */
