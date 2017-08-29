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

#ifndef MONGOC_WRITE_CONCERN_PRIVATE_H
#define MONGOC_WRITE_CONCERN_PRIVATE_H

#if !defined(MONGOC_COMPILATION)
#error "Only <mongoc.h> can be included directly."
#endif

#include <bson.h>


BSON_BEGIN_DECLS


#define MONGOC_WRITE_CONCERN_FSYNC_DEFAULT -1
#define MONGOC_WRITE_CONCERN_JOURNAL_DEFAULT -1


struct _mongoc_write_concern_t {
   int8_t fsync_; /* deprecated */
   int8_t journal;
   int32_t w;
   int32_t wtimeout;
   char *wtag;
   bool frozen;
   bson_t compiled;
   bson_t compiled_gle;
   bool is_default;
};


mongoc_write_concern_t *
_mongoc_write_concern_new_from_iter (bson_iter_t *iter);
bool
_mongoc_write_concern_iter_is_valid (bson_iter_t *iter);
const bson_t *
_mongoc_write_concern_get_gle (mongoc_write_concern_t *write_concern);
const bson_t *
_mongoc_write_concern_get_bson (mongoc_write_concern_t *write_concern);
bool
_mongoc_write_concern_validate (const mongoc_write_concern_t *write_concern,
                                bson_error_t *error);
bool
_mongoc_parse_wc_err (const bson_t *doc, bson_error_t *error);

BSON_END_DECLS


#endif /* MONGOC_WRITE_CONCERN_PRIVATE_H */
