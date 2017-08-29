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

#ifndef MONGOC_SOCKET_H
#define MONGOC_SOCKET_H

#if !defined(MONGOC_INSIDE) && !defined(MONGOC_COMPILATION)
#error "Only <mongoc.h> can be included directly."
#endif

#include <bson.h>
#include "mongoc-macros.h"
#include "mongoc-config.h"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <poll.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <sys/un.h>
#endif

#include "mongoc-iovec.h"


BSON_BEGIN_DECLS


typedef MONGOC_SOCKET_ARG3 mongoc_socklen_t;

typedef struct _mongoc_socket_t mongoc_socket_t;

typedef struct {
   mongoc_socket_t *socket;
   int events;
   int revents;
} mongoc_socket_poll_t;

MONGOC_EXPORT (mongoc_socket_t *)
mongoc_socket_accept (mongoc_socket_t *sock, int64_t expire_at);
MONGOC_EXPORT (int)
mongoc_socket_bind (mongoc_socket_t *sock,
                    const struct sockaddr *addr,
                    mongoc_socklen_t addrlen);
MONGOC_EXPORT (int)
mongoc_socket_close (mongoc_socket_t *socket);
MONGOC_EXPORT (int)
mongoc_socket_connect (mongoc_socket_t *sock,
                       const struct sockaddr *addr,
                       mongoc_socklen_t addrlen,
                       int64_t expire_at);
MONGOC_EXPORT (char *)
mongoc_socket_getnameinfo (mongoc_socket_t *sock);
MONGOC_EXPORT (void)
mongoc_socket_destroy (mongoc_socket_t *sock);
MONGOC_EXPORT (int)
mongoc_socket_errno (mongoc_socket_t *sock);
MONGOC_EXPORT (int)
mongoc_socket_getsockname (mongoc_socket_t *sock,
                           struct sockaddr *addr,
                           mongoc_socklen_t *addrlen);
MONGOC_EXPORT (int)
mongoc_socket_listen (mongoc_socket_t *sock, unsigned int backlog);
MONGOC_EXPORT (mongoc_socket_t *)
mongoc_socket_new (int domain, int type, int protocol);
MONGOC_EXPORT (ssize_t)
mongoc_socket_recv (mongoc_socket_t *sock,
                    void *buf,
                    size_t buflen,
                    int flags,
                    int64_t expire_at);
MONGOC_EXPORT (int)
mongoc_socket_setsockopt (mongoc_socket_t *sock,
                          int level,
                          int optname,
                          const void *optval,
                          mongoc_socklen_t optlen);
MONGOC_EXPORT (ssize_t)
mongoc_socket_send (mongoc_socket_t *sock,
                    const void *buf,
                    size_t buflen,
                    int64_t expire_at);
MONGOC_EXPORT (ssize_t)
mongoc_socket_sendv (mongoc_socket_t *sock,
                     mongoc_iovec_t *iov,
                     size_t iovcnt,
                     int64_t expire_at);
MONGOC_EXPORT (bool)
mongoc_socket_check_closed (mongoc_socket_t *sock);
MONGOC_EXPORT (void)
mongoc_socket_inet_ntop (struct addrinfo *rp, char *buf, size_t buflen);
MONGOC_EXPORT (ssize_t)
mongoc_socket_poll (mongoc_socket_poll_t *sds, size_t nsds, int32_t timeout);


BSON_END_DECLS


#endif /* MONGOC_SOCKET_H */
