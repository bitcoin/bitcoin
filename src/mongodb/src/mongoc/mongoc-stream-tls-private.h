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

#ifndef MONGOC_STREAM_TLS_PRIVATE_H
#define MONGOC_STREAM_TLS_PRIVATE_H

#if !defined(MONGOC_INSIDE) && !defined(MONGOC_COMPILATION)
#error "Only <mongoc.h> can be included directly."
#endif

#include <bson.h>

#include "mongoc-ssl.h"
#include "mongoc-stream.h"

BSON_BEGIN_DECLS

/**
 * mongoc_stream_tls_t:
 *
 * Overloaded mongoc_stream_t with additional TLS handshake and verification
 * callbacks.
 *
 */
struct _mongoc_stream_tls_t {
   mongoc_stream_t parent;       /* The TLS stream wrapper */
   mongoc_stream_t *base_stream; /* The underlying actual stream */
   void *ctx; /* TLS lib specific configuration or wrappers */
   int32_t timeout_msec;
   mongoc_ssl_opt_t ssl_opts;
   bool (*handshake) (mongoc_stream_t *stream,
                      const char *host,
                      int *events /* OUT*/,
                      bson_error_t *error);
};


BSON_END_DECLS

#endif /* MONGOC_STREAM_TLS_PRIVATE_H */
