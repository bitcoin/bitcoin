// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <crypto/hmac_pbkdf2.h>

#include <crypto/hmac_sha512.h>
#include <support/cleanse.h>

#include <cstring>

void PBKDF2_HMAC_SHA512(
    const unsigned char* password, size_t password_len,
    const unsigned char* salt, size_t salt_len,
    uint32_t iterations,
    unsigned char* output, size_t output_len)
{
    constexpr size_t HLEN = CHMAC_SHA512::OUTPUT_SIZE; // 64

    unsigned char U[HLEN];
    unsigned char T[HLEN];
    unsigned char block_index[4];

    size_t produced = 0;
    uint32_t i = 1;

    if (output_len == 0 || output == nullptr || iterations == 0) {
        return;
    }

    while (produced < output_len) {

        // INT_32_BE(i)
        block_index[0] = (i >> 24) & 0xff;
        block_index[1] = (i >> 16) & 0xff;
        block_index[2] = (i >> 8)  & 0xff;
        block_index[3] = i & 0xff;

        // U1 = HMAC(password, salt || INT_32_BE(i))
        CHMAC_SHA512(password, password_len)
            .Write(salt, salt_len)
            .Write(block_index, sizeof(block_index))
            .Finalize(U);

        memcpy(T, U, HLEN);

        // U2 ... U_iterations
        for (uint32_t j = 1; j < iterations; ++j) {

            // Uj = HMAC(password, Uj-1)
            CHMAC_SHA512(password, password_len)
                .Write(U, HLEN)
                .Finalize(U);

            // T_i ^= U_j
            for (size_t k = 0; k < HLEN; ++k) {
                T[k] ^= U[k];
            }
        }

        // Copy T_i into the derived key.
        size_t remaining = output_len - produced;
        size_t to_copy = remaining < HLEN ? remaining : HLEN;

        memcpy(output + produced, T, to_copy);

        produced += to_copy;
        ++i;
    }

    memory_cleanse(U, sizeof(U));
    memory_cleanse(T, sizeof(T));
    memory_cleanse(block_index, sizeof(block_index));
}