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

#ifndef MONGOC_SOCKET_PRIVATE_H
#define MONGOC_SOCKET_PRIVATE_H

#if !defined(MONGOC_INSIDE) && !defined(MONGOC_COMPILATION)
#error "Only <mongoc.h> can be included directly."
#endif

#include "mongoc-socket.h"

BSON_BEGIN_DECLS

struct _mongoc_socket_t {
#ifdef _WIN32
   SOCKET sd;
#else
   int sd;
#endif
   int errno_;
   int domain;
   int pid;
};

mongoc_socket_t *
mongoc_socket_accept_ex (mongoc_socket_t *sock,
                         int64_t expire_at,
                         uint16_t *port);

BSON_END_DECLS

#endif /* MONGOC_SOCKET_PRIVATE_H */
