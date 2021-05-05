// Copyright (c) 2014-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CRYPTO_RIPEMD160_H
#define BITCOIN_CRYPTO_RIPEMD160_H

#include <stdint.h>
#include <stdlib.h>

#include <uint256.h> // For uint160

/** A hasher class for RIPEMD-160. */
class CRIPEMD160
{
private:
    uint32_t s[5];
    unsigned char buf[64];
    uint64_t bytes;

public:
    static const size_t OUTPUT_SIZE = 20;

    CRIPEMD160();
    CRIPEMD160& Write(const unsigned char* data, size_t len);
    void Finalize(unsigned char hash[OUTPUT_SIZE]);
    CRIPEMD160& Reset();
};

/** Compute the 160-bit RIPEMD-160 hash of an array. */
inline uint160 RipeMd160(const unsigned char* data, size_t size)
{
    uint160 result;
    CRIPEMD160().Write(data, size).Finalize(result.begin());
    return result;
}

/** Compute the 160-bit RIPEMD-160 hash of an object. */
template <typename T>
inline uint160 RipeMd160(const T& container)
{
    return RipeMd160(container.data(), container.size());
}

#endif // BITCOIN_CRYPTO_RIPEMD160_H
