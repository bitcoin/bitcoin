// Copyright (c) 2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <crypto/hmac_sha256.h>

#include <algorithm>
#include <array>
#include <cstring>

CHMAC_SHA256::CHMAC_SHA256(const unsigned char* key, size_t keylen)
{
    std::array<unsigned char, OUTPUT_SIZE * 2> rkey{};
    if (keylen <= rkey.size()) {
        std::memcpy(rkey.data(), key, keylen);
    } else {
        CSHA256().Write(key, keylen).Finalize(rkey.data());
        std::fill(rkey.begin() + OUTPUT_SIZE, rkey.end(), 0);
    }

    for (auto n = 0u; n < rkey.size(); n++)
        rkey[n] ^= 0x5c;
    outer.Write(rkey.data(), rkey.size());

    for (auto n = 0u; n < rkey.size(); n++)
        rkey[n] ^= 0x5c ^ 0x36;
    inner.Write(rkey.data(), rkey.size());
}

void CHMAC_SHA256::Finalize(unsigned char hash[OUTPUT_SIZE])
{
    std::array<unsigned char, OUTPUT_SIZE> temp{};
    inner.Finalize(temp.data());
    outer.Write(temp.data(), temp.size()).Finalize(hash);
}
