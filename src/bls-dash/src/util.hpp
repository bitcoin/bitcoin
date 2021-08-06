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

#ifndef SRC_BLSUTIL_HPP_
#define SRC_BLSUTIL_HPP_

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

namespace bls {

class BLS;

class Bytes {
    const uint8_t* pData;
    const size_t nSize;

public:
    explicit Bytes(const uint8_t* pDataIn, const size_t nSizeIn)
        : pData(pDataIn), nSize(nSizeIn)
    {
    }
    explicit Bytes(const std::vector<uint8_t>& vecBytes)
        : pData(vecBytes.data()), nSize(vecBytes.size())
    {
    }

    inline const uint8_t* begin() const { return pData; }
    inline const uint8_t* end() const { return pData + nSize; }

    inline size_t size() const { return nSize; }

    const uint8_t& operator[](const int nIndex) const { return pData[nIndex]; }
};

class Util {
 public:
    typedef void *(*SecureAllocCallback)(size_t);
    typedef void (*SecureFreeCallback)(void*);
 public:
    static void Hash256(uint8_t* output, const uint8_t* message,
                        size_t messageLen) {
        md_map_sh256(output, message, messageLen);
    }

    static std::string HexStr(const uint8_t* data, size_t len) {
        std::stringstream s;
        s << std::hex;
        for (size_t i=0; i < len; ++i)
            s << std::setw(2) << std::setfill('0') << static_cast<int>(data[i]);
        return s.str();
    }

    static std::string HexStr(const std::vector<uint8_t> &data) {
        std::stringstream s;
        s << std::hex;
        for (size_t i=0; i < data.size(); ++i)
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
     * Converts one hex character to an int.
     */
    static uint8_t char2int(const char input) {
        if(input >= '0' && input <= '9')
            return input - '0';
        if(input >= 'A' && input <= 'F')
            return input - 'A' + 10;
        if(input >= 'a' && input <= 'f')
            return input - 'a' + 10;
        throw std::invalid_argument("Invalid input string");
    }

    /*
     * Converts a hex string into a vector of bytes.
     */
    static std::vector<uint8_t> HexToBytes(const std::string hex) {
        if (hex.size() % 2 != 0) {
            throw std::invalid_argument("Invalid input string, length must be multple of 2");
        }
        std::vector<uint8_t> ret = std::vector<uint8_t>();
        size_t start_at = 0;
        if (hex.rfind("0x", 0) == 0 || hex.rfind("0x", 0) == 0) {
            start_at = 2;
        }

        for (size_t i = start_at; i < hex.size(); i += 2) {
            ret.push_back(char2int(hex[i]) * 16 + char2int(hex[i+1]));
        }
        return ret;
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
