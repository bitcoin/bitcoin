/* Copyright 2016 MongoDB, Inc.
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

#ifdef MONGOC_ENABLE_CRYPTO_CNG
#include "mongoc-crypto-private.h"
#include "mongoc-crypto-cng-private.h"
#include "mongoc-log.h"

#include <windows.h>
#include <stdio.h>
#include <bcrypt.h>

#define NT_SUCCESS(Status) (((NTSTATUS) (Status)) >= 0)
#define STATUS_UNSUCCESSFUL ((NTSTATUS) 0xC0000001L)


bool
_mongoc_crypto_cng_hmac_or_hash (BCRYPT_ALG_HANDLE algorithm,
                                 void *key,
                                 size_t key_length,
                                 void *data,
                                 size_t data_length,
                                 void *mac_out)
{
   char *hash_object_buffer = 0;
   ULONG hash_object_length = 0;
   BCRYPT_HASH_HANDLE hash = 0;
   ULONG mac_length = 0;
   NTSTATUS status = STATUS_UNSUCCESSFUL;
   bool retval = false;
   ULONG noop = 0;

   status = BCryptGetProperty (algorithm,
                               BCRYPT_OBJECT_LENGTH,
                               (char *) &hash_object_length,
                               sizeof hash_object_length,
                               &noop,
                               0);

   if (!NT_SUCCESS (status)) {
      MONGOC_ERROR ("BCryptGetProperty(): OBJECT_LENGTH %x", status);
      return false;
   }

   status = BCryptGetProperty (algorithm,
                               BCRYPT_HASH_LENGTH,
                               (char *) &mac_length,
                               sizeof mac_length,
                               &noop,
                               0);

   if (!NT_SUCCESS (status)) {
      MONGOC_ERROR ("BCryptGetProperty(): HASH_LENGTH %x", status);
      return false;
   }

   hash_object_buffer = bson_malloc (hash_object_length);

   status = BCryptCreateHash (algorithm,
                              &hash,
                              hash_object_buffer,
                              hash_object_length,
                              key,
                              (ULONG) key_length,
                              0);

   if (!NT_SUCCESS (status)) {
      MONGOC_ERROR ("BCryptCreateHash(): %x", status);
      goto cleanup;
   }

   status = BCryptHashData (hash, data, (ULONG) data_length, 0);
   if (!NT_SUCCESS (status)) {
      MONGOC_ERROR ("BCryptHashData(): %x", status);
      goto cleanup;
   }

   status = BCryptFinishHash (hash, mac_out, mac_length, 0);
   if (!NT_SUCCESS (status)) {
      MONGOC_ERROR ("BCryptFinishHash(): %x", status);
      goto cleanup;
   }

   retval = true;

cleanup:
   if (hash) {
      (void) BCryptDestroyHash (hash);
   }

   bson_free (hash_object_buffer);
   return retval;
}

void
mongoc_crypto_cng_hmac_sha1 (mongoc_crypto_t *crypto,
                             const void *key,
                             int key_len,
                             const unsigned char *d,
                             int n,
                             unsigned char *md /* OUT */)
{
   static BCRYPT_ALG_HANDLE algorithm = 0;
   NTSTATUS status = STATUS_UNSUCCESSFUL;

   if (!algorithm) {
      status = BCryptOpenAlgorithmProvider (
         &algorithm, BCRYPT_SHA1_ALGORITHM, NULL, BCRYPT_ALG_HANDLE_HMAC_FLAG);
      if (!NT_SUCCESS (status)) {
         MONGOC_ERROR ("BCryptOpenAlgorithmProvider(): %x", status);
         return;
      }
   }

   _mongoc_crypto_cng_hmac_or_hash (algorithm, key, key_len, d, n, md);
}

bool
mongoc_crypto_cng_sha1 (mongoc_crypto_t *crypto,
                        const unsigned char *input,
                        const size_t input_len,
                        unsigned char *output /* OUT */)
{
   static BCRYPT_ALG_HANDLE algorithm = 0;
   NTSTATUS status = STATUS_UNSUCCESSFUL;

   if (!algorithm) {
      status = BCryptOpenAlgorithmProvider (
         &algorithm, BCRYPT_SHA1_ALGORITHM, NULL, 0);
      if (!NT_SUCCESS (status)) {
         MONGOC_ERROR ("BCryptOpenAlgorithmProvider(): %x", status);
         return false;
      }
   }

   return _mongoc_crypto_cng_hmac_or_hash (
      algorithm, NULL, 0, input, input_len, output);
}
#endif
