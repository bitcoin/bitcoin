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

#ifndef MONGOC_ERRNO_PRIVATE_H
#define MONGOC_ERRNO_PRIVATE_H

#if !defined(MONGOC_COMPILATION)
#error "Only <mongoc.h> can be included directly."
#endif

#include <bson.h>
#include <errno.h>
#ifdef _WIN32
#include <winsock2.h>
#include <winerror.h>
#endif


BSON_BEGIN_DECLS


#if defined(_WIN32)
#define MONGOC_ERRNO_IS_AGAIN(errno) \
   ((errno == EAGAIN) || (errno == WSAEWOULDBLOCK) || (errno == WSAEINPROGRESS))
#elif defined(__sun)
/* for some reason, accept() returns -1 and errno of 0 */
#define MONGOC_ERRNO_IS_AGAIN(errno)                                   \
   ((errno == EINTR) || (errno == EAGAIN) || (errno == EWOULDBLOCK) || \
    (errno == EINPROGRESS) || (errno == 0))
#else
#define MONGOC_ERRNO_IS_AGAIN(errno)                                   \
   ((errno == EINTR) || (errno == EAGAIN) || (errno == EWOULDBLOCK) || \
    (errno == EINPROGRESS))
#endif


BSON_END_DECLS


#endif /* MONGOC_ERRNO_PRIVATE_H */
