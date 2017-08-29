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

#ifndef TEST_CONVENIENCES_H
#define TEST_CONVENIENCES_H

#include <bson.h>

#include "mongoc.h"

bson_t *
tmp_bson (const char *json, ...);

void
bson_iter_bson (const bson_iter_t *iter, bson_t *bson);


#ifndef PATH_MAX
#define PATH_MAX 1024
#endif

#ifdef _WIN32
#define realpath(path, expanded) \
   GetFullPathName (path, PATH_MAX, expanded, NULL)
#endif


const char *
bson_lookup_utf8 (const bson_t *b, const char *key);

bool
bson_lookup_bool_null_ok (const bson_t *b, const char *key, bool default_value);

bool
bson_lookup_bool (const bson_t *b, const char *key, bool default_value);

void
bson_lookup_doc (const bson_t *b, const char *key, bson_t *doc);

void
bson_lookup_doc_null_ok (const bson_t *b, const char *key, bson_t *doc);

int32_t
bson_lookup_int32 (const bson_t *b, const char *key);

int64_t
bson_lookup_int64 (const bson_t *b, const char *key);

mongoc_write_concern_t *
bson_lookup_write_concern (const bson_t *b, const char *key);

mongoc_read_prefs_t *
bson_lookup_read_prefs (const bson_t *b, const char *key);

char *
single_quotes_to_double (const char *str);

typedef struct {
   char *errmsg;
   size_t errmsg_len;
   bool strict_numeric_types;
   char path[1000];
} match_ctx_t;

bool
match_bson (const bson_t *doc, const bson_t *pattern, bool is_command);

bool
match_bson_with_ctx (const bson_t *doc,
                     const bson_t *pattern,
                     bool is_command,
                     match_ctx_t *ctx);

bool
match_json (const bson_t *doc,
            bool is_command,
            const char *filename,
            int lineno,
            const char *funcname,
            const char *json_pattern,
            ...);

bool
mongoc_write_concern_append_bad (mongoc_write_concern_t *write_concern,
                                 bson_t *command);

#define FOUR_MB 1024 * 1024 * 4

const char *
huge_string (mongoc_client_t *client);

size_t
huge_string_length (mongoc_client_t *client);

const char *
four_mb_string ();

#define ASSERT_MATCH(doc, ...)                                                 \
   do {                                                                        \
      BSON_ASSERT (                                                            \
         match_json (doc, false, __FILE__, __LINE__, BSON_FUNC, __VA_ARGS__)); \
   } while (0)

#endif /* TEST_CONVENIENCES_H */
