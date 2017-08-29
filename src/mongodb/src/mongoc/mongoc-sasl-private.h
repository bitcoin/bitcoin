/*
 * Copyright 2017 MongoDB, Inc.
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

#ifndef MONGOC_SASL_PRIVATE_H
#define MONGOC_SASL_PRIVATE_H

#if !defined(MONGOC_COMPILATION)
#error "Only <mongoc.h> can be included directly."
#endif

#include <bson.h>
#include "mongoc-uri.h"
#include "mongoc-stream-private.h"
#include "mongoc-stream.h"
#include "mongoc-stream-socket.h"

BSON_BEGIN_DECLS


typedef struct {
   char *user;
   char *pass;
   char *service_name;
   char *service_host;
   bool canonicalize_host_name;
   char *mechanism;
} mongoc_sasl_t;


void
_mongoc_sasl_set_pass (mongoc_sasl_t *sasl, const char *pass);
void
_mongoc_sasl_set_user (mongoc_sasl_t *sasl, const char *user);
bool
_mongoc_sasl_set_mechanism (mongoc_sasl_t *sasl,
                            const char *mechanism,
                            bson_error_t *error);
void
_mongoc_sasl_set_service_name (mongoc_sasl_t *sasl, const char *service_name);
void
_mongoc_sasl_set_service_host (mongoc_sasl_t *sasl, const char *service_host);
void
_mongoc_sasl_set_properties (mongoc_sasl_t *sasl, const mongoc_uri_t *uri);
bool
_mongoc_sasl_get_canonicalized_name (mongoc_stream_t *node_stream, /* IN */
                                     char *name,                   /* OUT */
                                     size_t namelen,               /* IN */
                                     bson_error_t *error);         /* OUT */

BSON_END_DECLS


#endif /* MONGOC_SASL_PRIVATE_H */
