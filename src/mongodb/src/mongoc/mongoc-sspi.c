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
 *
 * This file originates from https://github.com/mongodb-labs/winkerberos
 */

#include "mongoc-config.h"

#ifdef MONGOC_ENABLE_SASL_SSPI

/* mingw doesn't define this */
#ifndef CRYPT_STRING_NOCRLF
#define CRYPT_STRING_NOCRLF 0x40000000
#endif

#include "mongoc-util-private.h"
#include "mongoc-sspi-private.h"

void
_mongoc_sspi_destroy_sspi_client_state (mongoc_sspi_client_state_t *state)
{
   if (state->haveCtx) {
      DeleteSecurityContext (&state->ctx);
      state->haveCtx = 0;
   }
   if (state->haveCred) {
      FreeCredentialsHandle (&state->cred);
      state->haveCred = 0;
   }
   if (state->spn != NULL) {
      free (state->spn);
      state->spn = NULL;
   }
   if (state->response != NULL) {
      free (state->response);
      state->response = NULL;
   }
   if (state->username != NULL) {
      free (state->username);
      state->username = NULL;
   }
}

void
_mongoc_sspi_set_gsserror (DWORD errCode, const SEC_CHAR *msg)
{
   SEC_CHAR *err;
   DWORD status;
   DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                 FORMAT_MESSAGE_IGNORE_INSERTS;
   status = FormatMessageA (flags,
                            NULL,
                            errCode,
                            MAKELANGID (LANG_NEUTRAL, SUBLANG_DEFAULT),
                            (LPTSTR) &err,
                            0,
                            NULL);
   if (status) {
      MONGOC_ERROR ("SSPI: %s: %s", msg, err);
      LocalFree (err);
   } else {
      MONGOC_ERROR ("SSPI: %s", msg);
   }
}

static SEC_CHAR *
_mongoc_sspi_base64_encode (const SEC_CHAR *value, DWORD vlen)
{
   SEC_CHAR *out = NULL;
   DWORD len;
   /* Get the correct size for the out buffer. */
   if (CryptBinaryToStringA ((BYTE *) value,
                             vlen,
                             CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF,
                             NULL,
                             &len)) {
      out = (SEC_CHAR *) malloc (sizeof (SEC_CHAR) * len);
      if (out) {
         /* Encode to the out buffer. */
         if (CryptBinaryToStringA ((BYTE *) value,
                                   vlen,
                                   CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF,
                                   out,
                                   &len)) {
            return out;
         } else {
            free (out);
         }
      }
   }
   MONGOC_ERROR ("%s", "CryptBinaryToString failed.");
   return NULL;
}

static SEC_CHAR *
_mongoc_sspi_base64_decode (const SEC_CHAR *value, DWORD *rlen)
{
   SEC_CHAR *out = NULL;
   /* Get the correct size for the out buffer. */
   if (CryptStringToBinaryA (
          value, 0, CRYPT_STRING_BASE64, NULL, rlen, NULL, NULL)) {
      out = (SEC_CHAR *) malloc (sizeof (SEC_CHAR) * *rlen);
      if (out) {
         /* Decode to the out buffer. */
         if (CryptStringToBinaryA (value,
                                   0,
                                   CRYPT_STRING_BASE64,
                                   (BYTE *) out,
                                   rlen,
                                   NULL,
                                   NULL)) {
            return out;
         } else {
            free (out);
         }
      }
   }
   MONGOC_ERROR ("%s", "CryptStringToBinary failed.");
   return NULL;
}

static CHAR *
_mongoc_sspi_wide_to_utf8 (WCHAR *value)
{
   CHAR *out;
   int len = WideCharToMultiByte (CP_UTF8, 0, value, -1, NULL, 0, NULL, NULL);
   if (len) {
      out = (CHAR *) malloc (sizeof (CHAR) * len);
      if (WideCharToMultiByte (CP_UTF8, 0, value, -1, out, len, NULL, NULL)) {
         return out;
      } else {
         free (out);
      }
   }
   _mongoc_sspi_set_gsserror (GetLastError (), "WideCharToMultiByte");
   return NULL;
}

int
_mongoc_sspi_auth_sspi_client_init (WCHAR *service,
                                    ULONG flags,
                                    WCHAR *user,
                                    ULONG ulen,
                                    WCHAR *domain,
                                    ULONG dlen,
                                    WCHAR *password,
                                    ULONG plen,
                                    mongoc_sspi_client_state_t *state)
{
   SECURITY_STATUS status;
   SEC_WINNT_AUTH_IDENTITY_W authIdentity;
   TimeStamp ignored;

   state->response = NULL;
   state->username = NULL;
   state->qop = SECQOP_WRAP_NO_ENCRYPT;
   state->flags = flags;
   state->haveCred = 0;
   state->haveCtx = 0;
   state->spn = _wcsdup (service);
   if (state->spn == NULL) {
      return MONGOC_SSPI_AUTH_GSS_ERROR;
   }
   /* Convert RFC-2078 format to SPN */
   if (!wcschr (state->spn, L'/')) {
      WCHAR *ptr = wcschr (state->spn, L'@');
      if (ptr) {
         *ptr = L'/';
      }
   }

   if (user) {
      authIdentity.User = user;
      authIdentity.UserLength = ulen;
      authIdentity.Domain = domain;
      authIdentity.DomainLength = dlen;
      authIdentity.Password = password;
      authIdentity.PasswordLength = plen;
      authIdentity.Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;
   }

   /* Note that the first paramater, pszPrincipal, appears to be
    * completely ignored in the Kerberos SSP. For more details see
    * https://github.com/mongodb-labs/winkerberos/issues/11.
    * */
   status = AcquireCredentialsHandleW (/* Principal */
                                       NULL,
                                       /* Security package name */
                                       L"kerberos",
                                       /* Credentials Use */
                                       SECPKG_CRED_OUTBOUND,
                                       /* LogonID (We don't use this) */
                                       NULL,
                                       /* AuthData */
                                       user ? &authIdentity : NULL,
                                       /* Always NULL */
                                       NULL,
                                       /* Always NULL */
                                       NULL,
                                       /* CredHandle */
                                       &state->cred,
                                       /* Expiry (Required but unused by us) */
                                       &ignored);
   if (status != SEC_E_OK) {
      _mongoc_sspi_set_gsserror (status, "AcquireCredentialsHandle");
      return MONGOC_SSPI_AUTH_GSS_ERROR;
   }
   state->haveCred = 1;
   return MONGOC_SSPI_AUTH_GSS_COMPLETE;
}

int
_mongoc_sspi_auth_sspi_client_step (mongoc_sspi_client_state_t *state,
                                    SEC_CHAR *challenge)
{
   SecBufferDesc inbuf;
   SecBuffer inBufs[1];
   SecBufferDesc outbuf;
   SecBuffer outBufs[1];
   ULONG ignored;
   SECURITY_STATUS status = MONGOC_SSPI_AUTH_GSS_CONTINUE;
   DWORD len;

   if (state->response != NULL) {
      free (state->response);
      state->response = NULL;
   }

   inbuf.ulVersion = SECBUFFER_VERSION;
   inbuf.cBuffers = 1;
   inbuf.pBuffers = inBufs;
   inBufs[0].pvBuffer = NULL;
   inBufs[0].cbBuffer = 0;
   inBufs[0].BufferType = SECBUFFER_TOKEN;
   if (state->haveCtx) {
      inBufs[0].pvBuffer = _mongoc_sspi_base64_decode (challenge, &len);
      if (!inBufs[0].pvBuffer) {
         return MONGOC_SSPI_AUTH_GSS_ERROR;
      }
      inBufs[0].cbBuffer = len;
   }

   outbuf.ulVersion = SECBUFFER_VERSION;
   outbuf.cBuffers = 1;
   outbuf.pBuffers = outBufs;
   outBufs[0].pvBuffer = NULL;
   outBufs[0].cbBuffer = 0;
   outBufs[0].BufferType = SECBUFFER_TOKEN;

   status = InitializeSecurityContextW (/* CredHandle */
                                        &state->cred,
                                        /* CtxtHandle (NULL on first call) */
                                        state->haveCtx ? &state->ctx : NULL,
                                        /* Service Principal Name */
                                        state->spn,
                                        /* Flags */
                                        ISC_REQ_ALLOCATE_MEMORY | state->flags,
                                        /* Always 0 */
                                        0,
                                        /* Target data representation */
                                        SECURITY_NETWORK_DREP,
                                        /* Challenge (NULL on first call) */
                                        state->haveCtx ? &inbuf : NULL,
                                        /* Always 0 */
                                        0,
                                        /* CtxtHandle (Set on first call) */
                                        &state->ctx,
                                        /* Output */
                                        &outbuf,
                                        /* Context attributes */
                                        &ignored,
                                        /* Expiry (We don't use this) */
                                        NULL);
   if (status != SEC_E_OK && status != SEC_I_CONTINUE_NEEDED) {
      _mongoc_sspi_set_gsserror (status, "InitializeSecurityContext");
      status = MONGOC_SSPI_AUTH_GSS_ERROR;
      goto done;
   }
   state->haveCtx = 1;
   if (outBufs[0].cbBuffer) {
      state->response =
         _mongoc_sspi_base64_encode (outBufs[0].pvBuffer, outBufs[0].cbBuffer);
      if (!state->response) {
         status = MONGOC_SSPI_AUTH_GSS_ERROR;
         goto done;
      }
   }
   if (status == SEC_E_OK) {
      /* Get authenticated username. */
      SecPkgContext_NamesW names;
      status = QueryContextAttributesW (&state->ctx, SECPKG_ATTR_NAMES, &names);
      if (status != SEC_E_OK) {
         _mongoc_sspi_set_gsserror (status, "QueryContextAttributesW");
         status = MONGOC_SSPI_AUTH_GSS_ERROR;
         goto done;
      }
      state->username = _mongoc_sspi_wide_to_utf8 (names.sUserName);
      if (state->username == NULL) {
         FreeContextBuffer (names.sUserName);
         status = MONGOC_SSPI_AUTH_GSS_ERROR;
         goto done;
      }
      FreeContextBuffer (names.sUserName);
      status = MONGOC_SSPI_AUTH_GSS_COMPLETE;
   } else {
      status = MONGOC_SSPI_AUTH_GSS_CONTINUE;
   }
done:
   if (inBufs[0].pvBuffer) {
      free (inBufs[0].pvBuffer);
   }
   if (outBufs[0].pvBuffer) {
      FreeContextBuffer (outBufs[0].pvBuffer);
   }
   return status;
}

int
_mongoc_sspi_auth_sspi_client_unwrap (mongoc_sspi_client_state_t *state,
                                      SEC_CHAR *challenge)
{
   SECURITY_STATUS status;
   DWORD len;
   SecBuffer wrapBufs[2];
   SecBufferDesc wrapBufDesc;
   wrapBufDesc.ulVersion = SECBUFFER_VERSION;
   wrapBufDesc.cBuffers = 2;
   wrapBufDesc.pBuffers = wrapBufs;

   if (state->response != NULL) {
      free (state->response);
      state->response = NULL;
      state->qop = SECQOP_WRAP_NO_ENCRYPT;
   }

   if (!state->haveCtx) {
      return MONGOC_SSPI_AUTH_GSS_ERROR;
   }

   wrapBufs[0].pvBuffer = _mongoc_sspi_base64_decode (challenge, &len);
   if (!wrapBufs[0].pvBuffer) {
      return MONGOC_SSPI_AUTH_GSS_ERROR;
   }
   wrapBufs[0].cbBuffer = len;
   wrapBufs[0].BufferType = SECBUFFER_STREAM;

   wrapBufs[1].pvBuffer = NULL;
   wrapBufs[1].cbBuffer = 0;
   wrapBufs[1].BufferType = SECBUFFER_DATA;

   status = DecryptMessage (&state->ctx, &wrapBufDesc, 0, &state->qop);
   if (status == SEC_E_OK) {
      status = MONGOC_SSPI_AUTH_GSS_COMPLETE;
   } else {
      _mongoc_sspi_set_gsserror (status, "DecryptMessage");
      status = MONGOC_SSPI_AUTH_GSS_ERROR;
      goto done;
   }
   if (wrapBufs[1].cbBuffer) {
      state->response = _mongoc_sspi_base64_encode (wrapBufs[1].pvBuffer,
                                                    wrapBufs[1].cbBuffer);
      if (!state->response) {
         status = MONGOC_SSPI_AUTH_GSS_ERROR;
      }
   }
done:
   if (wrapBufs[0].pvBuffer) {
      free (wrapBufs[0].pvBuffer);
   }
   return status;
}

int
_mongoc_sspi_auth_sspi_client_wrap (mongoc_sspi_client_state_t *state,
                                    SEC_CHAR *data,
                                    SEC_CHAR *user,
                                    ULONG ulen,
                                    int protect)
{
   SECURITY_STATUS status;
   SecPkgContext_Sizes sizes;
   SecBuffer wrapBufs[3];
   SecBufferDesc wrapBufDesc;
   SEC_CHAR *decodedData = NULL;
   SEC_CHAR *inbuf;
   SIZE_T inbufSize;
   SEC_CHAR *outbuf;
   DWORD outbufSize;
   SEC_CHAR *plaintextMessage;
   ULONG plaintextMessageSize;

   if (state->response != NULL) {
      free (state->response);
      state->response = NULL;
   }

   if (!state->haveCtx) {
      return MONGOC_SSPI_AUTH_GSS_ERROR;
   }

   status = QueryContextAttributes (&state->ctx, SECPKG_ATTR_SIZES, &sizes);
   if (status != SEC_E_OK) {
      _mongoc_sspi_set_gsserror (status, "QueryContextAttributes");
      return MONGOC_SSPI_AUTH_GSS_ERROR;
   }

   if (user) {
      /* Length of user + 4 bytes for security layer (see below). */
      plaintextMessageSize = ulen + 4;
   } else {
      decodedData = _mongoc_sspi_base64_decode (data, &plaintextMessageSize);
      if (!decodedData) {
         return MONGOC_SSPI_AUTH_GSS_ERROR;
      }
   }

   inbufSize =
      sizes.cbSecurityTrailer + plaintextMessageSize + sizes.cbBlockSize;
   inbuf = (SEC_CHAR *) malloc (inbufSize);
   if (inbuf == NULL) {
      free (decodedData);
      return MONGOC_SSPI_AUTH_GSS_ERROR;
   }

   plaintextMessage = inbuf + sizes.cbSecurityTrailer;
   if (user) {
      /* Authenticate the provided user. Unlike pykerberos, we don't
       * need any information from "data" to do that.
       * */
      plaintextMessage[0] = 1; /* No security layer */
      plaintextMessage[1] = 0;
      plaintextMessage[2] = 0;
      plaintextMessage[3] = 0;
      memcpy_s (plaintextMessage + 4,
                inbufSize - sizes.cbSecurityTrailer - 4,
                user,
                strlen (user));
   } else {
      /* No user provided. Just rewrap data. */
      memcpy_s (plaintextMessage,
                inbufSize - sizes.cbSecurityTrailer,
                decodedData,
                plaintextMessageSize);
      free (decodedData);
   }

   wrapBufDesc.cBuffers = 3;
   wrapBufDesc.pBuffers = wrapBufs;
   wrapBufDesc.ulVersion = SECBUFFER_VERSION;

   wrapBufs[0].cbBuffer = sizes.cbSecurityTrailer;
   wrapBufs[0].BufferType = SECBUFFER_TOKEN;
   wrapBufs[0].pvBuffer = inbuf;

   wrapBufs[1].cbBuffer = (ULONG) plaintextMessageSize;
   wrapBufs[1].BufferType = SECBUFFER_DATA;
   wrapBufs[1].pvBuffer = inbuf + sizes.cbSecurityTrailer;

   wrapBufs[2].cbBuffer = sizes.cbBlockSize;
   wrapBufs[2].BufferType = SECBUFFER_PADDING;
   wrapBufs[2].pvBuffer =
      inbuf + (sizes.cbSecurityTrailer + plaintextMessageSize);

   status = EncryptMessage (
      &state->ctx, protect ? 0 : SECQOP_WRAP_NO_ENCRYPT, &wrapBufDesc, 0);
   if (status != SEC_E_OK) {
      free (inbuf);
      _mongoc_sspi_set_gsserror (status, "EncryptMessage");
      return MONGOC_SSPI_AUTH_GSS_ERROR;
   }

   outbufSize =
      wrapBufs[0].cbBuffer + wrapBufs[1].cbBuffer + wrapBufs[2].cbBuffer;
   outbuf = (SEC_CHAR *) malloc (sizeof (SEC_CHAR) * outbufSize);
   memcpy_s (outbuf, outbufSize, wrapBufs[0].pvBuffer, wrapBufs[0].cbBuffer);
   memcpy_s (outbuf + wrapBufs[0].cbBuffer,
             outbufSize - wrapBufs[0].cbBuffer,
             wrapBufs[1].pvBuffer,
             wrapBufs[1].cbBuffer);
   memcpy_s (outbuf + wrapBufs[0].cbBuffer + wrapBufs[1].cbBuffer,
             outbufSize - wrapBufs[0].cbBuffer - wrapBufs[1].cbBuffer,
             wrapBufs[2].pvBuffer,
             wrapBufs[2].cbBuffer);
   state->response = _mongoc_sspi_base64_encode (outbuf, outbufSize);
   if (!state->response) {
      status = MONGOC_SSPI_AUTH_GSS_ERROR;
   } else {
      status = MONGOC_SSPI_AUTH_GSS_COMPLETE;
   }
   free (inbuf);
   free (outbuf);
   return status;
}

#endif
