// Copyright 2020 Chia Network Inc

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//    http://www.apache.org/licenses/LICENSE-2.0

// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef SRC_BLSHKDF_HPP_
#define SRC_BLSHKDF_HPP_

#include "relic_conf.h"
#include <math.h>

#if defined GMP && ARITH == GMP
#include <gmp.h>
#endif

#include <cassert>
#include "util.hpp"

namespace bls {

class HKDF256 {
    /**
     * Implements HKDF as specified in RFC5869: https://tools.ietf.org/html/rfc5869,
     * with sha256 as the hash function.
     **/
 public:
    static const uint8_t HASH_LEN = 32;

    static void Extract(uint8_t* prk_output, const uint8_t* salt, const size_t saltLen, const uint8_t* ikm, const size_t ikm_len) {
        // assert(saltLen == 4);  // Used for EIP2333 key derivation
        // assert(ikm_len == 32);  // Used for EIP2333 key derivation
        // Hash256 used as the hash function (sha256)
        // PRK Output is 32 bytes (HashLen)
        md_hmac(prk_output, ikm, ikm_len, salt, saltLen);
    }

    static void Expand(uint8_t* okm, size_t L, const uint8_t* prk, const uint8_t* info, const size_t infoLen) {
        assert(L <= 255 * HASH_LEN); // L <= 255 * HashLen
        assert(infoLen >= 0);
        size_t N = (L + HASH_LEN - 1) / HASH_LEN; // Round up
        size_t bytesWritten = 0;

        uint8_t* T = Util::SecAlloc<uint8_t>(HASH_LEN);
        uint8_t* hmacInput1 = Util::SecAlloc<uint8_t>(infoLen + 1);
        uint8_t* hmacInput = Util::SecAlloc<uint8_t>(HASH_LEN + infoLen + 1);

        assert(N >= 1 && N <= 255);

        for (size_t i = 1; i <= N; i++) {
            if (i == 1) {
                memcpy(hmacInput1, info, infoLen);
                hmacInput1[infoLen] = i;
                md_hmac(T, hmacInput1, infoLen + 1, prk, HASH_LEN);
            } else {
                memcpy(hmacInput, T, HASH_LEN);
                memcpy(hmacInput + HASH_LEN, info, infoLen);
                hmacInput[HASH_LEN + infoLen] = i;
                md_hmac(T, hmacInput, HASH_LEN + infoLen + 1, prk, HASH_LEN);
            }
            size_t to_write = L - bytesWritten;
            if (to_write > HASH_LEN) {
                to_write = HASH_LEN;
            }
            assert (to_write > 0 && to_write <= HASH_LEN);
            memcpy(okm + bytesWritten, T, to_write);
            bytesWritten += to_write;
        }
        Util::SecFree(T);
        Util::SecFree(hmacInput1);
        Util::SecFree(hmacInput);
        assert(bytesWritten == L);
    }

    static void ExtractExpand(uint8_t* output, size_t outputLen,
                              const uint8_t* key, size_t keyLen,
                              const uint8_t* salt, size_t saltLen,
                              const uint8_t* info, size_t infoLen) {
        uint8_t* prk = Util::SecAlloc<uint8_t>(HASH_LEN);
        HKDF256::Extract(prk, salt, saltLen, key, keyLen);
        HKDF256::Expand(output, outputLen, prk, info, infoLen);
        Util::SecFree(prk);
    }
};
} // end namespace bls
#endif  // SRC_BLSHKDF_HPP_
