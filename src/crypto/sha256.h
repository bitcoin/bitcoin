// Copyright (c) 2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CRYPTO_SHA256_H
#define BITCOIN_CRYPTO_SHA256_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/** A hasher class for SHA-256. */
class CSHA256
{
private:
    uint32_t s[8];
    unsigned char buf[64];
    size_t bytes;

public:
    static const size_t OUTPUT_SIZE = 32;

    CSHA256();
    ~CSHA256();
    CSHA256& Write(const unsigned char* data, size_t len);
    void Finalize(unsigned char hash[OUTPUT_SIZE]);
    CSHA256& Reset();
};

class CSHA256d
{
private:
    CSHA256 inner;

public:
    static const size_t OUTPUT_SIZE = CSHA256::OUTPUT_SIZE;

    CSHA256d& Write(const unsigned char* data, size_t len)
    {
        inner.Write(data, len);
        return *this;
    }

    void Finalize(unsigned char hash[OUTPUT_SIZE])
    {
        unsigned char buf[CSHA256::OUTPUT_SIZE];
        inner.Finalize(buf);
        inner.Reset().Write(buf, sizeof(buf)).Finalize(hash);
        memset(buf, 0, sizeof(buf));
    }

    CSHA256d& Reset() {
        inner.Reset();
        return *this;
    }
};

#endif // BITCOIN_CRYPTO_SHA256_H
