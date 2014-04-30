// Copyright (c) 2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_RIPEMD160_H
#define BITCOIN_RIPEMD160_H

#include <stdint.h>
#include <stdlib.h>

/** A hasher class for RIPEMD-160. */
class CRIPEMD160 {
private:
    uint32_t s[5];
    unsigned char buf[64];
    size_t bytes;

public:
    CRIPEMD160();
    CRIPEMD160& Write(const unsigned char *data, size_t len);
    void Finalize(unsigned char *hash);
    CRIPEMD160& Reset();
};

#endif
