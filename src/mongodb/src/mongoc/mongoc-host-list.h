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

#ifndef MONGOC_HOST_LIST_H
#define MONGOC_HOST_LIST_H

#if !defined(MONGOC_INSIDE) && !defined(MONGOC_COMPILATION)
#error "Only <mongoc.h> can be included directly."
#endif

#include <bson.h>


BSON_BEGIN_DECLS


#ifdef _POSIX_HOST_NAME_MAX
#define BSON_HOST_NAME_MAX _POSIX_HOST_NAME_MAX
#else
#define BSON_HOST_NAME_MAX 255
#endif


typedef struct _mongoc_host_list_t mongoc_host_list_t;


struct _mongoc_host_list_t {
   mongoc_host_list_t *next;
   char host[BSON_HOST_NAME_MAX + 1];
   char host_and_port[BSON_HOST_NAME_MAX + 7];
   uint16_t port;
   int family;
   void *padding[4];
};

BSON_END_DECLS


#endif /* MONGOC_HOST_LIST_H */
