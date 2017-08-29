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


#ifndef BSON_UTF8_H
#define BSON_UTF8_H


#if !defined(BSON_INSIDE) && !defined(BSON_COMPILATION)
#error "Only <bson.h> can be included directly."
#endif


#include "bson-macros.h"
#include "bson-types.h"


BSON_BEGIN_DECLS


BSON_EXPORT (bool)
bson_utf8_validate (const char *utf8, size_t utf8_len, bool allow_null);
BSON_EXPORT (char *)
bson_utf8_escape_for_json (const char *utf8, ssize_t utf8_len);
BSON_EXPORT (bson_unichar_t)
bson_utf8_get_char (const char *utf8);
BSON_EXPORT (const char *)
bson_utf8_next_char (const char *utf8);
BSON_EXPORT (void)
bson_utf8_from_unichar (bson_unichar_t unichar, char utf8[6], uint32_t *len);


BSON_END_DECLS


#endif /* BSON_UTF8_H */
