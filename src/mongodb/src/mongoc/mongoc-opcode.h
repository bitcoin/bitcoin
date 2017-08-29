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

#ifndef MONGOC_OPCODE_H
#define MONGOC_OPCODE_H

#if !defined(MONGOC_INSIDE) && !defined(MONGOC_COMPILATION)
#error "Only <mongoc.h> can be included directly."
#endif

#include <bson.h>


BSON_BEGIN_DECLS


typedef enum {
   MONGOC_OPCODE_REPLY = 1,
   MONGOC_OPCODE_MSG = 1000,
   MONGOC_OPCODE_UPDATE = 2001,
   MONGOC_OPCODE_INSERT = 2002,
   MONGOC_OPCODE_QUERY = 2004,
   MONGOC_OPCODE_GET_MORE = 2005,
   MONGOC_OPCODE_DELETE = 2006,
   MONGOC_OPCODE_KILL_CURSORS = 2007,
   MONGOC_OPCODE_COMPRESSED = 2012,
} mongoc_opcode_t;


BSON_END_DECLS


#endif /* MONGOC_OPCODE_H */
