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

#include "mongoc-config.h"

#ifdef MONGOC_ENABLE_SSL_LIBRESSL

#include <bson.h>

#include "mongoc-log.h"
#include "mongoc-trace-private.h"
#include "mongoc-ssl.h"
#include "mongoc-stream-tls.h"
#include "mongoc-stream-tls-private.h"
#include "mongoc-libressl-private.h"
#include "mongoc-stream-tls-libressl-private.h"

#include <tls.h>

#undef MONGOC_LOG_DOMAIN
#define MONGOC_LOG_DOMAIN "stream-libressl"


bool
mongoc_libressl_setup_certificate (mongoc_stream_tls_libressl_t *libressl,
                                   mongoc_ssl_opt_t *opt)
{
   uint8_t *file;
   size_t file_len;

   if (!opt->pem_file) {
      return false;
   }

   file = tls_load_file (opt->pem_file, &file_len, (char *) opt->pem_pwd);
   if (!file) {
      return false;
   }

   if (tls_config_set_keypair_mem (
          libressl->config, file, file_len, file, file_len) == -1) {
      MONGOC_ERROR ("%s", tls_config_error (libressl->config));
      return false;
   }

   return true;
}

bool
mongoc_libressl_setup_ca (mongoc_stream_tls_libressl_t *libressl,
                          mongoc_ssl_opt_t *opt)
{
   if (opt->ca_file) {
      tls_config_set_ca_file (libressl->config, opt->ca_file);
   }
   if (opt->ca_dir) {
      tls_config_set_ca_path (libressl->config, opt->ca_dir);
   }


   return true;
}

#endif
