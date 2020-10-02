// Copyright 2018 Chia Network Inc

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//    http://www.apache.org/licenses/LICENSE-2.0

// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef SRC_BLSUTIL_HPP_
#define SRC_BLSUTIL_HPP_

#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <cstdlib>

#include "relic_conf.h"

#if defined GMP && ARITH == GMP
#include <gmp.h>
#endif

#include "relic.h"
#include "relic_test.h"

namespace bls {

class BLS;

class Util {
 public:
    typedef void *(*SecureAllocCallback)(size_t);
    typedef void (*SecureFreeCallback)(void*);
 public:
    static void Hash256(uint8_t* output, const uint8_t* message,
                        size_t messageLen) {
        md_map_sh256(output, message, messageLen);
    }

    template<size_t S>
    struct BytesCompare {
        bool operator() (const uint8_t* lhs, const uint8_t* rhs) const {
            for (size_t i = 0; i < S; i++) {
                if (lhs[i] < rhs[i]) return true;
                if (lhs[i] > rhs[i]) return false;
            }
            return false;
        }
    };
    typedef struct BytesCompare<32> BytesCompare32;
    typedef struct BytesCompare<80> BytesCompare80;

    static std::string HexStr(const uint8_t* data, size_t len) {
        std::stringstream s;
        s << std::hex;
        for (int i=0; i < len; ++i)
            s << std::setw(2) << std::setfill('0') << static_cast<int>(data[i]);
        return s.str();
    }

    /*
     * Securely allocates a portion of memory, using libsodium. This prevents
     * paging to disk, and zeroes out the memory when it's freed.
     */
    template<class T>
    static T* SecAlloc(size_t numTs) {
        return static_cast<T*>(secureAllocCallback(sizeof(T) * numTs));
    }

    /*
     * Frees memory allocated using SecAlloc.
     */
    static void SecFree(void* ptr) {
        secureFreeCallback(ptr);
    }

    /*
     * Converts a 32 bit int to bytes.
     */
    static void IntToFourBytes(uint8_t* result,
                               const uint32_t input) {
        for (size_t i = 0; i < 4; i++) {
            result[3 - i] = (input >> (i * 8));
        }
    }

    /*
     * Converts a byte array to a 32 bit int.
     */
    static uint32_t FourBytesToInt(const uint8_t* bytes) {
        uint32_t sum = 0;
        for (size_t i = 0; i < 4; i++) {
            uint32_t addend = bytes[i] << (8 * (3 - i));
            sum += addend;
        }
        return sum;
    }

 private:
    friend class BLS;
    static SecureAllocCallback secureAllocCallback;
    static SecureFreeCallback secureFreeCallback;
};
} // end namespace bls
#endif  // SRC_BLSUTIL_HPP_
