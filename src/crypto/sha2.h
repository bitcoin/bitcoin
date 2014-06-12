// Copyright (c) 2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_SHA2_H
#define BITCOIN_SHA2_H

#include <stdint.h>
#include <stdlib.h>

/** A hasher class for SHA-256. */
class CSHA256 {
private:
    uint32_t s[8];
    unsigned char buf[64];
    size_t bytes;

public:
    static const size_t OUTPUT_SIZE = 32;

    CSHA256();
    CSHA256& Write(const unsigned char *data, size_t len);
    void Finalize(unsigned char hash[OUTPUT_SIZE]);
    CSHA256& Reset();
};

/** A hasher class for SHA-512. */
class CSHA512 {
private:
    uint64_t s[8];
    unsigned char buf[128];
    size_t bytes;

public:
    static const size_t OUTPUT_SIZE = 64;

    CSHA512();
    CSHA512& Write(const unsigned char *data, size_t len);
    void Finalize(unsigned char hash[OUTPUT_SIZE]);
    CSHA512& Reset();
};

/** A hasher class for HMAC-SHA-512. */
class CHMAC_SHA512 {
private:
    CSHA512 outer;
    CSHA512 inner;

public:
    static const size_t OUTPUT_SIZE = 64;

    CHMAC_SHA512(const unsigned char *key, size_t keylen);
    CHMAC_SHA512& Write(const unsigned char *data, size_t len) {
        inner.Write(data, len);
        return *this;
    }
    void Finalize(unsigned char hash[OUTPUT_SIZE]);
};

#endif
