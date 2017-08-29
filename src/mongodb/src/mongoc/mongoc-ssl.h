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

#ifndef MONGOC_SSL_H
#define MONGOC_SSL_H

#if !defined(MONGOC_INSIDE) && !defined(MONGOC_COMPILATION)
#error "Only <mongoc.h> can be included directly."
#endif

#include <bson.h>

#include "mongoc-macros.h"

BSON_BEGIN_DECLS


typedef struct _mongoc_ssl_opt_t mongoc_ssl_opt_t;


struct _mongoc_ssl_opt_t {
   const char *pem_file;
   const char *pem_pwd;
   const char *ca_file;
   const char *ca_dir;
   const char *crl_file;
   bool weak_cert_validation;
   bool allow_invalid_hostname;
   void *padding[7];
};


MONGOC_EXPORT (const mongoc_ssl_opt_t *)
mongoc_ssl_opt_get_default (void) BSON_GNUC_CONST;


BSON_END_DECLS


#endif /* MONGOC_SSL_H */
