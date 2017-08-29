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
#include "mongoc-crypto-private.h"

#ifdef MONGOC_ENABLE_CRYPTO_COMMON_CRYPTO
#include "mongoc-crypto-common-crypto-private.h"
#include <CommonCrypto/CommonHMAC.h>
#include <CommonCrypto/CommonDigest.h>


void
mongoc_crypto_common_crypto_hmac_sha1 (mongoc_crypto_t *crypto,
                                       const void *key,
                                       int key_len,
                                       const unsigned char *d,
                                       int n,
                                       unsigned char *md /* OUT */)
{
   /* U1 = HMAC(input, salt + 0001) */
   CCHmac (kCCHmacAlgSHA1, key, key_len, d, n, md);
}

bool
mongoc_crypto_common_crypto_sha1 (mongoc_crypto_t *crypto,
                                  const unsigned char *input,
                                  const size_t input_len,
                                  unsigned char *output /* OUT */)
{
   if (CC_SHA1 (input, input_len, output)) {
      return true;
   }
   return false;
}


#endif
