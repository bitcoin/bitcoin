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

#ifdef MONGOC_ENABLE_SSL_SECURE_CHANNEL

#include <bson.h>

#include "mongoc-log.h"
#include "mongoc-trace-private.h"
#include "mongoc-ssl.h"
#include "mongoc-stream-tls.h"
#include "mongoc-stream-tls-private.h"
#include "mongoc-secure-channel-private.h"
#include "mongoc-stream-tls-secure-channel-private.h"
#include "mongoc-errno-private.h"


#undef MONGOC_LOG_DOMAIN
#define MONGOC_LOG_DOMAIN "stream-secure-channel"

/* mingw doesn't define this */
#ifndef SECBUFFER_ALERT
#define SECBUFFER_ALERT 17
#endif


PCCERT_CONTEXT
mongoc_secure_channel_setup_certificate_from_file (const char *filename)
{
   char *pem;
   FILE *file;
   bool success;
   HCRYPTKEY hKey;
   long pem_length;
   HCRYPTPROV provider;
   CERT_BLOB public_blob;
   const char *pem_public;
   const char *pem_private;
   LPBYTE blob_private = NULL;
   PCCERT_CONTEXT cert = NULL;
   DWORD blob_private_len = 0;
   HCERTSTORE cert_store = NULL;
   DWORD encrypted_cert_len = 0;
   LPBYTE encrypted_cert = NULL;
   DWORD encrypted_private_len = 0;
   LPBYTE encrypted_private = NULL;


   file = fopen (filename, "rb");
   if (!file) {
      MONGOC_ERROR ("Couldn't open file '%s'", filename);
      return false;
   }

   fseek (file, 0, SEEK_END);
   pem_length = ftell (file);
   fseek (file, 0, SEEK_SET);
   if (pem_length < 1) {
      MONGOC_ERROR ("Couldn't determine file size of '%s'", filename);
      return false;
   }

   pem = (char *) bson_malloc0 (pem_length);
   fread ((void *) pem, 1, pem_length, file);
   fclose (file);

   pem_public = strstr (pem, "-----BEGIN CERTIFICATE-----");
   pem_private = strstr (pem, "-----BEGIN ENCRYPTED PRIVATE KEY-----");

   if (pem_private) {
      MONGOC_ERROR ("Detected unsupported encrypted private key");
      goto fail;
   }

   pem_private = strstr (pem, "-----BEGIN RSA PRIVATE KEY-----");
   if (!pem_private) {
      pem_private = strstr (pem, "-----BEGIN PRIVATE KEY-----");
   }

   if (!pem_private) {
      MONGOC_ERROR ("Can't find private key in '%s'", filename);
      goto fail;
   }

   public_blob.cbData = (DWORD) strlen (pem_public);
   public_blob.pbData = (BYTE *) pem_public;

   /* https://msdn.microsoft.com/en-us/library/windows/desktop/aa380264%28v=vs.85%29.aspx
    */
   CryptQueryObject (
      CERT_QUERY_OBJECT_BLOB,      /* dwObjectType, blob or file */
      &public_blob,                /* pvObject, Unicode filename */
      CERT_QUERY_CONTENT_FLAG_ALL, /* dwExpectedContentTypeFlags */
      CERT_QUERY_FORMAT_FLAG_ALL,  /* dwExpectedFormatTypeFlags */
      0,                           /* dwFlags, reserved for "future use" */
      NULL,                        /* pdwMsgAndCertEncodingType, OUT, unused */
      NULL, /* pdwContentType (dwExpectedContentTypeFlags), OUT, unused */
      NULL, /* pdwFormatType (dwExpectedFormatTypeFlags,), OUT, unused */
      NULL, /* phCertStore, OUT, HCERTSTORE.., unused, for now */
      NULL, /* phMsg, OUT, HCRYPTMSG, only for PKC7, unused */
      (const void **) &cert /* ppvContext, OUT, the Certificate Context */
      );

   if (!cert) {
      MONGOC_ERROR ("Failed to extract public key from '%s'. Error 0x%.8X",
                    filename,
                    GetLastError ());
      goto fail;
   }

   /* https://msdn.microsoft.com/en-us/library/windows/desktop/aa380285%28v=vs.85%29.aspx
    */
   success =
      CryptStringToBinaryA (pem_private,               /* pszString */
                            0,                         /* cchString */
                            CRYPT_STRING_BASE64HEADER, /* dwFlags */
                            NULL,                      /* pbBinary */
                            &encrypted_private_len,    /* pcBinary, IN/OUT */
                            NULL,                      /* pdwSkip */
                            NULL);                     /* pdwFlags */
   if (!success) {
      MONGOC_ERROR ("Failed to convert base64 private key. Error 0x%.8X",
                    GetLastError ());
      goto fail;
   }

   encrypted_private = (LPBYTE) bson_malloc0 (encrypted_private_len);
   success = CryptStringToBinaryA (pem_private,
                                   0,
                                   CRYPT_STRING_BASE64HEADER,
                                   encrypted_private,
                                   &encrypted_private_len,
                                   NULL,
                                   NULL);
   if (!success) {
      MONGOC_ERROR ("Failed to convert base64 private key. Error 0x%.8X",
                    GetLastError ());
      goto fail;
   }

   /* https://msdn.microsoft.com/en-us/library/windows/desktop/aa379912%28v=vs.85%29.aspx
    */
   success = CryptDecodeObjectEx (
      X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, /* dwCertEncodingType */
      PKCS_RSA_PRIVATE_KEY,                    /* lpszStructType */
      encrypted_private,                       /* pbEncoded */
      encrypted_private_len,                   /* cbEncoded */
      0,                                       /* dwFlags */
      NULL,                                    /* pDecodePara */
      NULL,                                    /* pvStructInfo */
      &blob_private_len);                      /* pcbStructInfo */
   if (!success) {
      LPTSTR msg = NULL;
      FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER |
                        FORMAT_MESSAGE_FROM_SYSTEM |
                        FORMAT_MESSAGE_ARGUMENT_ARRAY,
                     NULL,
                     GetLastError (),
                     LANG_NEUTRAL,
                     (LPTSTR) &msg,
                     0,
                     NULL);
      MONGOC_ERROR (
         "Failed to parse private key. %s (0x%.8X)", msg, GetLastError ());
      LocalFree (msg);
      goto fail;
   }

   blob_private = (LPBYTE) bson_malloc0 (blob_private_len);
   success = CryptDecodeObjectEx (X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                                  PKCS_RSA_PRIVATE_KEY,
                                  encrypted_private,
                                  encrypted_private_len,
                                  0,
                                  NULL,
                                  blob_private,
                                  &blob_private_len);
   if (!success) {
      MONGOC_ERROR ("Failed to parse private key. Error 0x%.8X",
                    GetLastError ());
      goto fail;
   }

   /* https://msdn.microsoft.com/en-us/library/windows/desktop/aa379886%28v=vs.85%29.aspx
    */
   success = CryptAcquireContext (&provider,            /* phProv */
                                  NULL,                 /* pszContainer */
                                  MS_ENHANCED_PROV,     /* pszProvider */
                                  PROV_RSA_FULL,        /* dwProvType */
                                  CRYPT_VERIFYCONTEXT); /* dwFlags */
   if (!success) {
      MONGOC_ERROR ("CryptAcquireContext failed with error 0x%.8X",
                    GetLastError ());
      goto fail;
   }

   /* https://msdn.microsoft.com/en-us/library/windows/desktop/aa380207%28v=vs.85%29.aspx
    */
   success = CryptImportKey (provider,         /* hProv */
                             blob_private,     /* pbData */
                             blob_private_len, /* dwDataLen */
                             0,                /* hPubKey */
                             0,                /* dwFlags */
                             &hKey);           /* phKey, OUT */
   if (!success) {
      MONGOC_ERROR ("CryptImportKey for private key failed with error 0x%.8X",
                    GetLastError ());
      goto fail;
   }

   /* https://msdn.microsoft.com/en-us/library/windows/desktop/aa376573%28v=vs.85%29.aspx
    */
   success = CertSetCertificateContextProperty (
      cert,                         /* pCertContext */
      CERT_KEY_PROV_HANDLE_PROP_ID, /* dwPropId */
      0,                            /* dwFlags */
      (const void *) provider);     /* pvData */
   if (success) {
      TRACE ("Successfully loaded client certificate");
      return cert;
   }

   MONGOC_ERROR ("Can't associate private key with public key: 0x%.8X",
                 GetLastError ());

fail:
   SecureZeroMemory (pem, pem_length);
   bson_free (pem);
   if (encrypted_private) {
      SecureZeroMemory (encrypted_private, encrypted_private_len);
      bson_free (encrypted_private);
   }

   if (blob_private) {
      SecureZeroMemory (blob_private, blob_private_len);
      bson_free (blob_private);
   }

   return NULL;
}

PCCERT_CONTEXT
mongoc_secure_channel_setup_certificate (
   mongoc_stream_tls_secure_channel_t *secure_channel, mongoc_ssl_opt_t *opt)
{
   return mongoc_secure_channel_setup_certificate_from_file (opt->pem_file);
}

void
_bson_append_szoid (bson_string_t *retval,
                    PCCERT_CONTEXT cert,
                    const char *label,
                    void *oid)
{
   DWORD oid_len =
      CertGetNameString (cert, CERT_NAME_ATTR_TYPE, 0, oid, NULL, 0);

   if (oid_len > 1) {
      char *tmp = bson_malloc0 (oid_len);

      CertGetNameString (cert, CERT_NAME_ATTR_TYPE, 0, oid, tmp, oid_len);
      bson_string_append_printf (retval, "%s%s", label, tmp);
      bson_free (tmp);
   }
}
char *
_mongoc_secure_channel_extract_subject (const char *filename,
                                        const char *passphrase)
{
   bson_string_t *retval;
   PCCERT_CONTEXT cert;

   cert = mongoc_secure_channel_setup_certificate_from_file (filename);
   if (!cert) {
      return NULL;
   }

   retval = bson_string_new ("");
   ;
   _bson_append_szoid (retval, cert, "C=", szOID_COUNTRY_NAME);
   _bson_append_szoid (retval, cert, ",ST=", szOID_STATE_OR_PROVINCE_NAME);
   _bson_append_szoid (retval, cert, ",L=", szOID_LOCALITY_NAME);
   _bson_append_szoid (retval, cert, ",O=", szOID_ORGANIZATION_NAME);
   _bson_append_szoid (retval, cert, ",OU=", szOID_ORGANIZATIONAL_UNIT_NAME);
   _bson_append_szoid (retval, cert, ",CN=", szOID_COMMON_NAME);
   _bson_append_szoid (retval, cert, ",STREET=", szOID_STREET_ADDRESS);

   return bson_string_free (retval, false);
}

bool
mongoc_secure_channel_setup_ca (
   mongoc_stream_tls_secure_channel_t *secure_channel, mongoc_ssl_opt_t *opt)
{
   FILE *file;
   long length;
   const char *pem_key;
   HCERTSTORE cert_store = NULL;
   PCCERT_CONTEXT cert = NULL;
   DWORD encrypted_cert_len = 0;
   LPBYTE encrypted_cert = NULL;

   file = fopen (opt->ca_file, "rb");
   if (!file) {
      MONGOC_WARNING ("Couldn't open file '%s'", opt->ca_file);
      return false;
   }

   fseek (file, 0, SEEK_END);
   length = ftell (file);
   fseek (file, 0, SEEK_SET);
   if (length < 1) {
      MONGOC_WARNING ("Couldn't determine file size of '%s'", opt->ca_file);
      return false;
   }

   pem_key = (const char *) bson_malloc0 (length);
   fread ((void *) pem_key, 1, length, file);
   fclose (file);

   /* If we have private keys or other fuzz, seek to the good stuff */
   pem_key = strstr (pem_key, "-----BEGIN CERTIFICATE-----");
   /*printf ("%s\n", pem_key);*/

   if (!pem_key) {
      MONGOC_WARNING ("Couldn't find certificate in '%d'", opt->ca_file);
      return false;
   }

   if (!CryptStringToBinaryA (pem_key,
                              0,
                              CRYPT_STRING_BASE64HEADER,
                              NULL,
                              &encrypted_cert_len,
                              NULL,
                              NULL)) {
      MONGOC_ERROR ("Failed to convert BASE64 public key. Error 0x%.8X",
                    GetLastError ());
      return false;
   }

   encrypted_cert = (LPBYTE) LocalAlloc (0, encrypted_cert_len);
   if (!CryptStringToBinaryA (pem_key,
                              0,
                              CRYPT_STRING_BASE64HEADER,
                              encrypted_cert,
                              &encrypted_cert_len,
                              NULL,
                              NULL)) {
      MONGOC_ERROR ("Failed to convert BASE64 public key. Error 0x%.8X",
                    GetLastError ());
      return false;
   }

   cert = CertCreateCertificateContext (
      X509_ASN_ENCODING, encrypted_cert, encrypted_cert_len);
   if (!cert) {
      MONGOC_WARNING ("Could not convert certificate");
      return false;
   }


   cert_store = CertOpenStore (
      CERT_STORE_PROV_SYSTEM,                  /* provider */
      X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, /* certificate encoding */
      0,                                       /* unused */
      CERT_SYSTEM_STORE_LOCAL_MACHINE,         /* dwFlags */
      L"Root"); /* system store name. "My" or "Root" */

   if (cert_store == NULL) {
      MONGOC_ERROR ("Error opening certificate store");
      return false;
   }

   if (CertAddCertificateContextToStore (
          cert_store, cert, CERT_STORE_ADD_USE_EXISTING, NULL)) {
      TRACE ("Added the certificate !");
      CertCloseStore (cert_store, 0);
      return true;
   }
   MONGOC_WARNING ("Failed adding the cert");
   CertCloseStore (cert_store, 0);

   return false;
}

bool
mongoc_secure_channel_setup_crl (
   mongoc_stream_tls_secure_channel_t *secure_channel, mongoc_ssl_opt_t *opt)
{
   HCERTSTORE cert_store = NULL;
   PCCERT_CONTEXT cert = NULL;
   LPWSTR str;
   int chars;

   chars = MultiByteToWideChar (CP_ACP, 0, opt->crl_file, -1, NULL, 0);
   if (chars < 1) {
      MONGOC_WARNING ("Can't determine opt->crl_file length");
      return false;
   }
   str = (LPWSTR) bson_malloc0 (chars);
   MultiByteToWideChar (CP_ACP, 0, opt->crl_file, -1, str, chars);


   /* https://msdn.microsoft.com/en-us/library/windows/desktop/aa380264%28v=vs.85%29.aspx
    */
   CryptQueryObject (
      CERT_QUERY_OBJECT_FILE,      /* dwObjectType, blob or file */
      str,                         /* pvObject, Unicode filename */
      CERT_QUERY_CONTENT_FLAG_CRL, /* dwExpectedContentTypeFlags */
      CERT_QUERY_FORMAT_FLAG_ALL,  /* dwExpectedFormatTypeFlags */
      0,                           /* dwFlags, reserved for "future use" */
      NULL,                        /* pdwMsgAndCertEncodingType, OUT, unused */
      NULL, /* pdwContentType (dwExpectedContentTypeFlags), OUT, unused */
      NULL, /* pdwFormatType (dwExpectedFormatTypeFlags,), OUT, unused */
      NULL, /* phCertStore, OUT, HCERTSTORE.., unused, for now */
      NULL, /* phMsg, OUT, HCRYPTMSG, only for PKC7, unused */
      (const void **) &cert /* ppvContext, OUT, the Certificate Context */
      );
   bson_free (str);

   if (!cert) {
      MONGOC_WARNING ("Can't extract CRL from '%s'", opt->crl_file);
      return false;
   }


   cert_store = CertOpenStore (
      CERT_STORE_PROV_SYSTEM,                  /* provider */
      X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, /* certificate encoding */
      0,                                       /* unused */
      CERT_SYSTEM_STORE_LOCAL_MACHINE,         /* dwFlags */
      L"Root"); /* system store name. "My" or "Root" */

   if (cert_store == NULL) {
      MONGOC_ERROR ("Error opening certificate store");
      CertFreeCertificateContext (cert);
      return false;
   }

   if (CertAddCertificateContextToStore (
          cert_store, cert, CERT_STORE_ADD_USE_EXISTING, NULL)) {
      TRACE ("Added the certificate !");
      CertFreeCertificateContext (cert);
      CertCloseStore (cert_store, 0);
      return true;
   }

   MONGOC_WARNING ("Failed adding the cert");
   CertFreeCertificateContext (cert);
   CertCloseStore (cert_store, 0);

   return false;
}

size_t
mongoc_secure_channel_read (mongoc_stream_tls_t *tls,
                            void *data,
                            size_t data_length)
{
   ssize_t length;

   errno = 0;
   TRACE ("Wanting to read: %d", data_length);
   /* 4th argument is minimum bytes, while the data_length is the
    * size of the buffer. We are totally fine with just one TLS record (few
    *bytes)
    **/
   length = mongoc_stream_read (
      tls->base_stream, data, data_length, 0, tls->timeout_msec);

   TRACE ("Got %d", length);

   if (length > 0) {
      return length;
   }

   return 0;
}

size_t
mongoc_secure_channel_write (mongoc_stream_tls_t *tls,
                             const void *data,
                             size_t data_length)
{
   ssize_t length;

   errno = 0;
   TRACE ("Wanting to write: %d", data_length);
   length = mongoc_stream_write (
      tls->base_stream, (void *) data, data_length, tls->timeout_msec);
   TRACE ("Wrote: %d", length);


   return length;
}

/**
 * The follow functions comes from one of my favorite project, cURL!
 * Thank you so much for having gone through the Secure Channel pain for me.
 *
 *
 * Copyright (C) 2012 - 2015, Marc Hoersken, <info@marc-hoersken.de>
 * Copyright (C) 2012, Mark Salisbury, <mark.salisbury@hp.com>
 * Copyright (C) 2012 - 2015, Daniel Stenberg, <daniel@haxx.se>, et al.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution. The terms
 * are also available at https://curl.haxx.se/docs/copyright.html.
 *
 * You may opt to use, copy, modify, merge, publish, distribute and/or sell
 * copies of the Software, and permit persons to whom the Software is
 * furnished to do so, under the terms of the COPYING file.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ***************************************************************************/

/*
 * Based upon the PolarSSL implementation in polarssl.c and polarssl.h:
 *   Copyright (C) 2010, 2011, Hoi-Ho Chan, <hoiho.chan@gmail.com>
 *
 * Based upon the CyaSSL implementation in cyassl.c and cyassl.h:
 *   Copyright (C) 1998 - 2012, Daniel Stenberg, <daniel@haxx.se>, et al.
 *
 * Thanks for code and inspiration!
 */

void
_mongoc_secure_channel_init_sec_buffer (SecBuffer *buffer,
                                        unsigned long buf_type,
                                        void *buf_data_ptr,
                                        unsigned long buf_byte_size)
{
   buffer->cbBuffer = buf_byte_size;
   buffer->BufferType = buf_type;
   buffer->pvBuffer = buf_data_ptr;
}

void
_mongoc_secure_channel_init_sec_buffer_desc (SecBufferDesc *desc,
                                             SecBuffer *buffer_array,
                                             unsigned long buffer_count)
{
   desc->ulVersion = SECBUFFER_VERSION;
   desc->pBuffers = buffer_array;
   desc->cBuffers = buffer_count;
}


bool
mongoc_secure_channel_handshake_step_1 (mongoc_stream_tls_t *tls,
                                        char *hostname)
{
   SecBuffer outbuf;
   ssize_t written = -1;
   SecBufferDesc outbuf_desc;
   SECURITY_STATUS sspi_status = SEC_E_OK;
   mongoc_stream_tls_secure_channel_t *secure_channel =
      (mongoc_stream_tls_secure_channel_t *) tls->ctx;

   TRACE ("SSL/TLS connection with '%s' (step 1/3)", hostname);

   /* setup output buffer */
   _mongoc_secure_channel_init_sec_buffer (&outbuf, SECBUFFER_EMPTY, NULL, 0);
   _mongoc_secure_channel_init_sec_buffer_desc (&outbuf_desc, &outbuf, 1);

   /* setup request flags */
   secure_channel->req_flags = ISC_REQ_SEQUENCE_DETECT | ISC_REQ_REPLAY_DETECT |
                               ISC_REQ_CONFIDENTIALITY |
                               ISC_REQ_ALLOCATE_MEMORY | ISC_REQ_STREAM;

   /* allocate memory for the security context handle */
   secure_channel->ctxt = (mongoc_secure_channel_ctxt *) bson_malloc0 (
      sizeof (mongoc_secure_channel_ctxt));

   /* https://msdn.microsoft.com/en-us/library/windows/desktop/aa375924.aspx */
   sspi_status = InitializeSecurityContext (
      &secure_channel->cred->cred_handle, /* phCredential */
      NULL,                               /* phContext */
      hostname,                           /* pszTargetName */
      secure_channel->req_flags,          /* fContextReq */
      0,                                  /* Reserved1, must be 0 */
      0,                                  /* TargetDataRep, unused */
      NULL,                               /* pInput */
      0,                                  /* Reserved2, must be 0 */
      &secure_channel->ctxt->ctxt_handle, /* phNewContext OUT param */
      &outbuf_desc,                       /* pOutput OUT param */
      &secure_channel->ret_flags,         /* pfContextAttr OUT param */
      &secure_channel->ctxt->time_stamp   /* ptsExpiry OUT param */
      );

   if (sspi_status != SEC_I_CONTINUE_NEEDED) {
      MONGOC_ERROR ("initial InitializeSecurityContext failed: %d",
                    sspi_status);
      return false;
   }

   TRACE ("sending initial handshake data: sending %lu bytes...",
          outbuf.cbBuffer);

   /* send initial handshake data which is now stored in output buffer */
   written =
      mongoc_secure_channel_write (tls, outbuf.pvBuffer, outbuf.cbBuffer);
   FreeContextBuffer (outbuf.pvBuffer);

   if (outbuf.cbBuffer != (size_t) written) {
      MONGOC_ERROR ("failed to send initial handshake data: "
                    "sent %zd of %lu bytes",
                    written,
                    outbuf.cbBuffer);
      return false;
   }

   TRACE ("sent initial handshake data: sent %zd bytes", written);

   secure_channel->recv_unrecoverable_err = 0;
   secure_channel->recv_sspi_close_notify = false;
   secure_channel->recv_connection_closed = false;

   /* continue to second handshake step */
   secure_channel->connecting_state = ssl_connect_2;

   return true;
}

bool
mongoc_secure_channel_handshake_step_2 (mongoc_stream_tls_t *tls,
                                        char *hostname)
{
   mongoc_stream_tls_secure_channel_t *secure_channel =
      (mongoc_stream_tls_secure_channel_t *) tls->ctx;
   SECURITY_STATUS sspi_status = SEC_E_OK;
   unsigned char *reallocated_buffer;
   ssize_t nread = -1, written = -1;
   size_t reallocated_length;
   SecBufferDesc outbuf_desc;
   SecBufferDesc inbuf_desc;
   SecBuffer outbuf[3];
   SecBuffer inbuf[2];
   bool doread;
   int i;

   doread = (secure_channel->connecting_state != ssl_connect_2_writing) ? true
                                                                        : false;

   TRACE ("SSL/TLS connection with endpoint (step 2/3)");

   if (!secure_channel->cred || !secure_channel->ctxt) {
      return false;
   }

   /* buffer to store previously received and decrypted data */
   if (secure_channel->decdata_buffer == NULL) {
      secure_channel->decdata_offset = 0;
      secure_channel->decdata_length = MONGOC_SCHANNEL_BUFFER_INIT_SIZE;
      secure_channel->decdata_buffer =
         bson_malloc0 (secure_channel->decdata_length);
   }

   /* buffer to store previously received and encrypted data */
   if (secure_channel->encdata_buffer == NULL) {
      secure_channel->encdata_offset = 0;
      secure_channel->encdata_length = MONGOC_SCHANNEL_BUFFER_INIT_SIZE;
      secure_channel->encdata_buffer =
         bson_malloc0 (secure_channel->encdata_length);
   }

   /* if we need a bigger buffer to read a full message, increase buffer now */
   if (secure_channel->encdata_length - secure_channel->encdata_offset <
       MONGOC_SCHANNEL_BUFFER_FREE_SIZE) {
      /* increase internal encrypted data buffer */
      reallocated_length =
         secure_channel->encdata_offset + MONGOC_SCHANNEL_BUFFER_FREE_SIZE;
      reallocated_buffer =
         bson_realloc (secure_channel->encdata_buffer, reallocated_length);

      secure_channel->encdata_buffer = reallocated_buffer;
      secure_channel->encdata_length = reallocated_length;
   }

   for (;;) {
      if (doread) {
         /* read encrypted handshake data from socket */
         nread = mongoc_secure_channel_read (
            tls,
            (char *) (secure_channel->encdata_buffer +
                      secure_channel->encdata_offset),
            secure_channel->encdata_length - secure_channel->encdata_offset);

         if (!nread) {
            if (MONGOC_ERRNO_IS_AGAIN (errno)) {
               if (secure_channel->connecting_state != ssl_connect_2_writing) {
                  secure_channel->connecting_state = ssl_connect_2_reading;
               }

               TRACE ("failed to receive handshake, need more data");
               return true;
            }

            MONGOC_ERROR (
               "failed to receive handshake, SSL/TLS connection failed");
            return false;
         }

         /* increase encrypted data buffer offset */
         secure_channel->encdata_offset += nread;
      }

      TRACE ("encrypted data buffer: offset %zu length %zu",
             secure_channel->encdata_offset,
             secure_channel->encdata_length);

      /* setup input buffers */
      _mongoc_secure_channel_init_sec_buffer (
         &inbuf[0],
         SECBUFFER_TOKEN,
         malloc (secure_channel->encdata_offset),
         (unsigned long) (secure_channel->encdata_offset &
                          (size_t) 0xFFFFFFFFUL));
      _mongoc_secure_channel_init_sec_buffer (
         &inbuf[1], SECBUFFER_EMPTY, NULL, 0);
      _mongoc_secure_channel_init_sec_buffer_desc (&inbuf_desc, inbuf, 2);

      /* setup output buffers */
      _mongoc_secure_channel_init_sec_buffer (
         &outbuf[0], SECBUFFER_TOKEN, NULL, 0);
      _mongoc_secure_channel_init_sec_buffer (
         &outbuf[1], SECBUFFER_ALERT, NULL, 0);
      _mongoc_secure_channel_init_sec_buffer (
         &outbuf[2], SECBUFFER_EMPTY, NULL, 0);
      _mongoc_secure_channel_init_sec_buffer_desc (&outbuf_desc, outbuf, 3);

      if (inbuf[0].pvBuffer == NULL) {
         MONGOC_ERROR ("unable to allocate memory");
         return false;
      }

      /* copy received handshake data into input buffer */
      memcpy (inbuf[0].pvBuffer,
              secure_channel->encdata_buffer,
              secure_channel->encdata_offset);

      /* https://msdn.microsoft.com/en-us/library/windows/desktop/aa375924.aspx
       */
      sspi_status =
         InitializeSecurityContext (&secure_channel->cred->cred_handle,
                                    &secure_channel->ctxt->ctxt_handle,
                                    hostname,
                                    secure_channel->req_flags,
                                    0,
                                    0,
                                    &inbuf_desc,
                                    0,
                                    NULL,
                                    &outbuf_desc,
                                    &secure_channel->ret_flags,
                                    &secure_channel->ctxt->time_stamp);

      /* free buffer for received handshake data */
      free (inbuf[0].pvBuffer);

      /* check if the handshake was incomplete */
      if (sspi_status == SEC_E_INCOMPLETE_MESSAGE) {
         secure_channel->connecting_state = ssl_connect_2_reading;
         TRACE ("received incomplete message, need more data");
         return true;
      }

      /* If the server has requested a client certificate, attempt to continue
       * the handshake without one. This will allow connections to servers which
       * request a client certificate but do not require it. */
      if (sspi_status == SEC_I_INCOMPLETE_CREDENTIALS &&
          !(secure_channel->req_flags & ISC_REQ_USE_SUPPLIED_CREDS)) {
         secure_channel->req_flags |= ISC_REQ_USE_SUPPLIED_CREDS;
         secure_channel->connecting_state = ssl_connect_2_writing;
         TRACE ("a client certificate has been requested");
         return true;
      }

      /* check if the handshake needs to be continued */
      if (sspi_status == SEC_I_CONTINUE_NEEDED || sspi_status == SEC_E_OK) {
         for (i = 0; i < 3; i++) {
            /* search for handshake tokens that need to be send */
            if (outbuf[i].BufferType == SECBUFFER_TOKEN &&
                outbuf[i].cbBuffer > 0) {
               TRACE ("sending next handshake data: sending %lu bytes...",
                      outbuf[i].cbBuffer);

               /* send handshake token to server */
               written = mongoc_secure_channel_write (
                  tls, outbuf[i].pvBuffer, outbuf[i].cbBuffer);

               if (outbuf[i].cbBuffer != (size_t) written) {
                  MONGOC_ERROR ("failed to send next handshake data: "
                                "sent %zd of %lu bytes",
                                written,
                                outbuf[i].cbBuffer);
                  return false;
               }
            }

            /* free obsolete buffer */
            if (outbuf[i].pvBuffer != NULL) {
               FreeContextBuffer (outbuf[i].pvBuffer);
            }
         }
      } else {
         switch (sspi_status) {
         case SEC_E_WRONG_PRINCIPAL:
            MONGOC_ERROR ("SSL Certification verification failed: hostname "
                          "doesn't match certificate");
            break;

         case SEC_E_UNTRUSTED_ROOT:
            MONGOC_ERROR ("SSL Certification verification failed: Untrusted "
                          "root certificate");
            break;

         case SEC_E_CERT_EXPIRED:
            MONGOC_ERROR ("SSL Certification verification failed: certificate "
                          "has expired");
            break;
         case CRYPT_E_NO_REVOCATION_CHECK:
            /* This seems to be raised also when hostname doesn't match the
             * certificate */
            MONGOC_ERROR ("SSL Certification verification failed: failed "
                          "revocation/hostname check");
            break;

         case SEC_E_INSUFFICIENT_MEMORY:
         case SEC_E_INTERNAL_ERROR:
         case SEC_E_INVALID_HANDLE:
         case SEC_E_INVALID_TOKEN:
         case SEC_E_LOGON_DENIED:
         case SEC_E_NO_AUTHENTICATING_AUTHORITY:
         case SEC_E_NO_CREDENTIALS:
         case SEC_E_TARGET_UNKNOWN:
         case SEC_E_UNSUPPORTED_FUNCTION:
#ifdef SEC_E_APPLICATION_PROTOCOL_MISMATCH
         /* Not available in VS2010 */
         case SEC_E_APPLICATION_PROTOCOL_MISMATCH:
#endif


         default: {
            LPTSTR msg = NULL;

            FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER |
                              FORMAT_MESSAGE_FROM_SYSTEM |
                              FORMAT_MESSAGE_ARGUMENT_ARRAY,
                           NULL,
                           GetLastError (),
                           LANG_NEUTRAL,
                           (LPTSTR) &msg,
                           0,
                           NULL);
            MONGOC_ERROR ("Failed to initialize security context, error code: "
                          "0x%04X%04X: %s",
                          (sspi_status >> 16) & 0xffff,
                          sspi_status & 0xffff,
                          msg);
            LocalFree (msg);
         }
         }
         return false;
      }

      /* check if there was additional remaining encrypted data */
      if (inbuf[1].BufferType == SECBUFFER_EXTRA && inbuf[1].cbBuffer > 0) {
         TRACE ("encrypted data length: %lu", inbuf[1].cbBuffer);

         /*
          * There are two cases where we could be getting extra data here:
          * 1) If we're renegotiating a connection and the handshake is already
          * complete (from the server perspective), it can encrypted app data
          * (not handshake data) in an extra buffer at this point.
          * 2) (sspi_status == SEC_I_CONTINUE_NEEDED) We are negotiating a
          * connection and this extra data is part of the handshake.
          * We should process the data immediately; waiting for the socket to
          * be ready may fail since the server is done sending handshake data.
          */
         /* check if the remaining data is less than the total amount
          * and therefore begins after the already processed data */
         if (secure_channel->encdata_offset > inbuf[1].cbBuffer) {
            memmove (secure_channel->encdata_buffer,
                     (secure_channel->encdata_buffer +
                      secure_channel->encdata_offset) -
                        inbuf[1].cbBuffer,
                     inbuf[1].cbBuffer);
            secure_channel->encdata_offset = inbuf[1].cbBuffer;

            if (sspi_status == SEC_I_CONTINUE_NEEDED) {
               doread = FALSE;
               continue;
            }
         }
      } else {
         secure_channel->encdata_offset = 0;
      }

      break;
   }

   /* check if the handshake needs to be continued */
   if (sspi_status == SEC_I_CONTINUE_NEEDED) {
      secure_channel->connecting_state = ssl_connect_2_reading;
      return true;
   }

   /* check if the handshake is complete */
   if (sspi_status == SEC_E_OK) {
      secure_channel->connecting_state = ssl_connect_3;
      TRACE ("SSL/TLS handshake complete\n");
   }

   return true;
}

bool
mongoc_secure_channel_handshake_step_3 (mongoc_stream_tls_t *tls,
                                        char *hostname)
{
   mongoc_stream_tls_secure_channel_t *secure_channel =
      (mongoc_stream_tls_secure_channel_t *) tls->ctx;

   BSON_ASSERT (ssl_connect_3 == secure_channel->connecting_state);

   TRACE ("SSL/TLS connection with %s (step 3/3)\n", hostname);

   if (!secure_channel->cred) {
      return false;
   }

   /* check if the required context attributes are met */
   if (secure_channel->ret_flags != secure_channel->req_flags) {
      MONGOC_ERROR ("Failed handshake");

      return false;
   }

   secure_channel->connecting_state = ssl_connect_done;

   return true;
}
#endif
