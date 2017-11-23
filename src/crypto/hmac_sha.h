// Copyright (c) 2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CRYPTO_HMAC_SHA_H
#define BITCOIN_CRYPTO_HMAC_SHA_H

#include <crypto/sha256.h>
#include <crypto/sha512.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <cstring>

/** A generic hasher class for HMAC-SHA-*. */
template <typename CSHA>
class CHMAC_SHA
{
private:
    CSHA outer{};
    CSHA inner{};

public:
    static constexpr const size_t OUTPUT_SIZE = CSHA::OUTPUT_SIZE;

    CHMAC_SHA(const unsigned char* key, size_t keylen)
    {
        std::array<unsigned char, OUTPUT_SIZE * 2> rkey{};
        if (keylen <= rkey.size()) {
            std::memcpy(rkey.data(), key, keylen);
            std::fill(rkey.begin() + keylen, rkey.end(), 0);
        } else {
            CSHA{}.Write(key, keylen).Finalize(rkey.data());
            std::fill(rkey.begin() + OUTPUT_SIZE, rkey.end(), 0);
        }

        std::transform(rkey.begin(), rkey.end(), rkey.begin(), [](const unsigned char c) { return c ^ 0x5c; });
        outer.Write(rkey.data(), rkey.size());

        std::transform(rkey.begin(), rkey.end(), rkey.begin(), [](const unsigned char c) { return c ^ (0x5c ^ 0x36); });
        inner.Write(rkey.data(), rkey.size());
    }

    CHMAC_SHA& Write(const unsigned char* data, size_t len)
    {
        inner.Write(data, len);
        return *this;
    }
    void Finalize(unsigned char hash[OUTPUT_SIZE])
    {
        std::array<unsigned char, OUTPUT_SIZE> temp{};
        inner.Finalize(temp.data());
        outer.Write(temp.data(), temp.size()).Finalize(hash);
    }
};

#endif // BITCOIN_CRYPTO_HMAC_SHA_H
