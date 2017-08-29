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

#ifndef MONGOC_STREAM_TLS_OPENSSL_BIO_PRIVATE_H
#define MONGOC_STREAM_TLS_OPENSSL_BIO_PRIVATE_H

#if !defined(MONGOC_INSIDE) && !defined(MONGOC_COMPILATION)
#error "Only <mongoc.h> can be included directly."
#endif

#ifdef MONGOC_ENABLE_SSL_OPENSSL
#include <bson.h>

#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

BSON_BEGIN_DECLS

BIO_METHOD *
mongoc_stream_tls_openssl_bio_meth_new ();

void
mongoc_stream_tls_openssl_bio_set_data (BIO *b, void *ptr);

int
mongoc_stream_tls_openssl_bio_create (BIO *b);

int
mongoc_stream_tls_openssl_bio_destroy (BIO *b);

int
mongoc_stream_tls_openssl_bio_read (BIO *b, char *buf, int len);

int
mongoc_stream_tls_openssl_bio_write (BIO *b, const char *buf, int len);

long
mongoc_stream_tls_openssl_bio_ctrl (BIO *b, int cmd, long num, void *ptr);

int
mongoc_stream_tls_openssl_bio_gets (BIO *b, char *buf, int len);

int
mongoc_stream_tls_openssl_bio_puts (BIO *b, const char *str);

BSON_END_DECLS

#endif /* MONGOC_ENABLE_SSL_OPENSSL */
#endif /* MONGOC_STREAM_TLS_OPENSSL_BIO_PRIVATE_H */
