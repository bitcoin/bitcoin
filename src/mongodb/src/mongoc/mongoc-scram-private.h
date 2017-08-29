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


#if !defined(MONGOC_COMPILATION)
#error "Only <mongoc.h> can be included directly."
#endif


#ifndef MONGOC_SCRAM_PRIVATE_H
#define MONGOC_SCRAM_PRIVATE_H


#include <bson.h>
#include "mongoc-crypto-private.h"


BSON_BEGIN_DECLS

#define MONGOC_SCRAM_HASH_SIZE 20

typedef struct _mongoc_scram_t {
   bool done;
   int step;
   char *user;
   char *pass;
   uint8_t client_key[MONGOC_SCRAM_HASH_SIZE];
   uint8_t server_key[MONGOC_SCRAM_HASH_SIZE];
   uint8_t salted_password[MONGOC_SCRAM_HASH_SIZE];
   char encoded_nonce[48];
   int32_t encoded_nonce_len;
   uint8_t *auth_message;
   uint32_t auth_messagemax;
   uint32_t auth_messagelen;
#ifdef MONGOC_ENABLE_CRYPTO
   mongoc_crypto_t crypto;
#endif
} mongoc_scram_t;

void
_mongoc_scram_startup ();

void
_mongoc_scram_init (mongoc_scram_t *scram);

void
_mongoc_scram_set_pass (mongoc_scram_t *scram, const char *pass);

void
_mongoc_scram_set_user (mongoc_scram_t *scram, const char *user);

void
_mongoc_scram_set_client_key (mongoc_scram_t *scram,
                              const uint8_t *client_key,
                              size_t len);

void
_mongoc_scram_set_server_key (mongoc_scram_t *scram,
                              const uint8_t *server_key,
                              size_t len);

void
_mongoc_scram_set_salted_password (mongoc_scram_t *scram,
                                   const uint8_t *salted_password,
                                   size_t len);

void
_mongoc_scram_destroy (mongoc_scram_t *scram);

bool
_mongoc_scram_step (mongoc_scram_t *scram,
                    const uint8_t *inbuf,
                    uint32_t inbuflen,
                    uint8_t *outbuf,
                    uint32_t outbufmax,
                    uint32_t *outbuflen,
                    bson_error_t *error);

BSON_END_DECLS


#endif /* MONGOC_SCRAM_PRIVATE_H */
