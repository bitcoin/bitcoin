/*
 * Copyright 2016 MongoDB, Inc.
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

#ifndef MONGOC_SSL_PRIVATE_H
#define MONGOC_SSL_PRIVATE_H

#if !defined(MONGOC_INSIDE) && !defined(MONGOC_COMPILATION)
#error "Only <mongoc.h> can be included directly."
#endif

#include <bson.h>
#include "mongoc-uri-private.h"


BSON_BEGIN_DECLS


char *
mongoc_ssl_extract_subject (const char *filename, const char *passphrase);

void
_mongoc_ssl_opts_from_uri (mongoc_ssl_opt_t *ssl_opt, mongoc_uri_t *uri);
void
_mongoc_ssl_opts_copy_to (const mongoc_ssl_opt_t *src, mongoc_ssl_opt_t *dst);
void
_mongoc_ssl_opts_cleanup (mongoc_ssl_opt_t *opt);

BSON_END_DECLS


#endif /* MONGOC_SSL_PRIVATE_H */
