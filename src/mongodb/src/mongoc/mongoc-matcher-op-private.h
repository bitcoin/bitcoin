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

#ifndef MONGOC_MATCHER_OP_PRIVATE_H
#define MONGOC_MATCHER_OP_PRIVATE_H

#if !defined(MONGOC_COMPILATION)
#error "Only <mongoc.h> can be included directly."
#endif

#include <bson.h>


BSON_BEGIN_DECLS


typedef union _mongoc_matcher_op_t mongoc_matcher_op_t;
typedef struct _mongoc_matcher_op_base_t mongoc_matcher_op_base_t;
typedef struct _mongoc_matcher_op_logical_t mongoc_matcher_op_logical_t;
typedef struct _mongoc_matcher_op_compare_t mongoc_matcher_op_compare_t;
typedef struct _mongoc_matcher_op_exists_t mongoc_matcher_op_exists_t;
typedef struct _mongoc_matcher_op_type_t mongoc_matcher_op_type_t;
typedef struct _mongoc_matcher_op_not_t mongoc_matcher_op_not_t;


typedef enum {
   MONGOC_MATCHER_OPCODE_EQ,
   MONGOC_MATCHER_OPCODE_GT,
   MONGOC_MATCHER_OPCODE_GTE,
   MONGOC_MATCHER_OPCODE_IN,
   MONGOC_MATCHER_OPCODE_LT,
   MONGOC_MATCHER_OPCODE_LTE,
   MONGOC_MATCHER_OPCODE_NE,
   MONGOC_MATCHER_OPCODE_NIN,
   MONGOC_MATCHER_OPCODE_OR,
   MONGOC_MATCHER_OPCODE_AND,
   MONGOC_MATCHER_OPCODE_NOT,
   MONGOC_MATCHER_OPCODE_NOR,
   MONGOC_MATCHER_OPCODE_EXISTS,
   MONGOC_MATCHER_OPCODE_TYPE,
} mongoc_matcher_opcode_t;


struct _mongoc_matcher_op_base_t {
   mongoc_matcher_opcode_t opcode;
};


struct _mongoc_matcher_op_logical_t {
   mongoc_matcher_op_base_t base;
   mongoc_matcher_op_t *left;
   mongoc_matcher_op_t *right;
};


struct _mongoc_matcher_op_compare_t {
   mongoc_matcher_op_base_t base;
   char *path;
   bson_iter_t iter;
};


struct _mongoc_matcher_op_exists_t {
   mongoc_matcher_op_base_t base;
   char *path;
   bool exists;
};


struct _mongoc_matcher_op_type_t {
   mongoc_matcher_op_base_t base;
   bson_type_t type;
   char *path;
};


struct _mongoc_matcher_op_not_t {
   mongoc_matcher_op_base_t base;
   mongoc_matcher_op_t *child;
   char *path;
};


union _mongoc_matcher_op_t {
   mongoc_matcher_op_base_t base;
   mongoc_matcher_op_logical_t logical;
   mongoc_matcher_op_compare_t compare;
   mongoc_matcher_op_exists_t exists;
   mongoc_matcher_op_type_t type;
   mongoc_matcher_op_not_t not_;
};


mongoc_matcher_op_t *
_mongoc_matcher_op_logical_new (mongoc_matcher_opcode_t opcode,
                                mongoc_matcher_op_t *left,
                                mongoc_matcher_op_t *right);
mongoc_matcher_op_t *
_mongoc_matcher_op_compare_new (mongoc_matcher_opcode_t opcode,
                                const char *path,
                                const bson_iter_t *iter);
mongoc_matcher_op_t *
_mongoc_matcher_op_exists_new (const char *path, bool exists);
mongoc_matcher_op_t *
_mongoc_matcher_op_type_new (const char *path, bson_type_t type);
mongoc_matcher_op_t *
_mongoc_matcher_op_not_new (const char *path, mongoc_matcher_op_t *child);
bool
_mongoc_matcher_op_match (mongoc_matcher_op_t *op, const bson_t *bson);
void
_mongoc_matcher_op_destroy (mongoc_matcher_op_t *op);
void
_mongoc_matcher_op_to_bson (mongoc_matcher_op_t *op, bson_t *bson);


BSON_END_DECLS


#endif /* MONGOC_MATCHER_OP_PRIVATE_H */
