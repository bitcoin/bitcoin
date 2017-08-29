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

#ifndef MONGOC_SSPI_PRIVATE_H
#define MONGOC_SSPI_PRIVATE_H

#if !defined(MONGOC_COMPILATION)
#error "Only <mongoc.h> can be included directly."
#endif

#include <bson.h>


BSON_BEGIN_DECLS


#define SECURITY_WIN32 1 /* Required for SSPI */

#include <Windows.h>
#include <limits.h>
#include <sspi.h>
#include <string.h>

#define MONGOC_SSPI_AUTH_GSS_ERROR -1
#define MONGOC_SSPI_AUTH_GSS_COMPLETE 1
#define MONGOC_SSPI_AUTH_GSS_CONTINUE 0

typedef struct {
   CredHandle cred;
   CtxtHandle ctx;
   WCHAR *spn;
   SEC_CHAR *response;
   SEC_CHAR *username;
   ULONG flags;
   UCHAR haveCred;
   UCHAR haveCtx;
   INT qop;
} mongoc_sspi_client_state_t;

void
_mongoc_sspi_set_gsserror (DWORD errCode, const SEC_CHAR *msg);

void
_mongoc_sspi_destroy_sspi_client_state (mongoc_sspi_client_state_t *state);

int
_mongoc_sspi_auth_sspi_client_init (WCHAR *service,
                                    ULONG flags,
                                    WCHAR *user,
                                    ULONG ulen,
                                    WCHAR *domain,
                                    ULONG dlen,
                                    WCHAR *password,
                                    ULONG plen,
                                    mongoc_sspi_client_state_t *state);
int
_mongoc_sspi_auth_sspi_client_step (mongoc_sspi_client_state_t *state,
                                    SEC_CHAR *challenge);

int
_mongoc_sspi_auth_sspi_client_unwrap (mongoc_sspi_client_state_t *state,
                                      SEC_CHAR *challenge);

int
_mongoc_sspi_auth_sspi_client_wrap (mongoc_sspi_client_state_t *state,
                                    SEC_CHAR *data,
                                    SEC_CHAR *user,
                                    ULONG ulen,
                                    INT protect);


BSON_END_DECLS


#endif /* MONGOC_SSPI_PRIVATE_H */
