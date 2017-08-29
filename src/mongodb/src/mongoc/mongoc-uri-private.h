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

#ifndef MONGOC_URI_PRIVATE_H
#define MONGOC_URI_PRIVATE_H

#if !defined(MONGOC_COMPILATION)
#error "Only <mongoc.h> can be included directly."
#endif

#include "mongoc-uri.h"


BSON_BEGIN_DECLS


bool
mongoc_uri_append_host (mongoc_uri_t *uri, const char *host, uint16_t port);
bool
mongoc_uri_parse_host (mongoc_uri_t *uri, const char *str, bool downcase);

int32_t
mongoc_uri_get_local_threshold_option (const mongoc_uri_t *uri);

BSON_END_DECLS


#endif /* MONGOC_URI_PRIVATE_H */
