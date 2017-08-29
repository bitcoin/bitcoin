/*
 * Copyright 2014 MongoDB, Inc.
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


#ifndef MONGOC_RAND_H
#define MONGOC_RAND_H


#include <bson.h>

#include "mongoc-macros.h"

BSON_BEGIN_DECLS

MONGOC_EXPORT (void)
mongoc_rand_seed (const void *buf, int num);
MONGOC_EXPORT (void)
mongoc_rand_add (const void *buf, int num, double entropy);
MONGOC_EXPORT (int)
mongoc_rand_status (void);

BSON_END_DECLS


#endif /* MONGOC_RAND_H */
