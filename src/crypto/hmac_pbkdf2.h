// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CRYPTO_HMAC_PBKDF2_H
#define BITCOIN_CRYPTO_HMAC_PBKDF2_H

#include <cstddef>
#include <cstdint>

/** PBKDF2 (RFC 8018 / PKCS#5 v2.1) key derivation using HMAC-SHA-512 as the
 *  pseudo-random function. Derives output_len bytes into output from password
 *  and salt over the given number of iterations. */
void PBKDF2_HMAC_SHA512(
    const unsigned char* password, size_t password_len,
    const unsigned char* salt, size_t salt_len,
    uint32_t iterations,
    unsigned char* output, size_t output_len);

#endif // BITCOIN_CRYPTO_HMAC_PBKDF2_H
