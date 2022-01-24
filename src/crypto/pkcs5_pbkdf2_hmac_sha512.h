// Copyright (c) 2022 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CRYPTO_PKCS5_PBKDF2_HMAC_SHA512_H
#define BITCOIN_CRYPTO_PKCS5_PBKDF2_HMAC_SHA512_H

#include <stdint.h>
#include <stdlib.h>

/** A rfc2898 implementation of PKCS#5 v2.0 password based encryption key
 * derivation function PBKDF2 with HMAC_SHA512. This implementation is
 * based on OpenSSL code with few refactorings.
 * https://github.com/openssl/openssl/blob/83b4a24384e62ed8cf91f51bf9a303f98017e13e/crypto/kdf/pbkdf2.c#L200-L264
 **/

class PKCS5_PBKDF2_HMAC_SHA512
{
public:
    PKCS5_PBKDF2_HMAC_SHA512(
    const char* pass, size_t passlen,
    const unsigned char* salt, int saltlen,
    int iter,
    size_t keylen, unsigned char* out);
};

#endif // BITCOIN_CRYPTO_PKCS5_PBKDF2_HMAC_SHA512_H

