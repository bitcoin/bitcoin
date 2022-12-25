// Copyright (c) 2018-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_UTIL_GOLOMBRICE_H
#define SYSCOIN_UTIL_GOLOMBRICE_H

#include <util/fastrange.h>

#include <streams.h>

#include <cstdint>

template <typename OStream>
void GolombRiceEncode(BitStreamWriter<OStream>& bitwriter, uint8_t P, uint64_t x)
{
    // Write quotient as unary-encoded: q 1's followed by one 0.
    uint64_t q = x >> P;
    while (q > 0) {
        int nbits = q <= 64 ? static_cast<int>(q) : 64;
        bitwriter.Write(~0ULL, nbits);
        q -= nbits;
    }
    bitwriter.Write(0, 1);

    // Write the remainder in P bits. Since the remainder is just the bottom
    // P bits of x, there is no need to mask first.
    bitwriter.Write(x, P);
}

template <typename IStream>
uint64_t GolombRiceDecode(BitStreamReader<IStream>& bitreader, uint8_t P)
{
    // Read unary-encoded quotient: q 1's followed by one 0.
    uint64_t q = 0;
    while (bitreader.Read(1) == 1) {
        ++q;
    }

    uint64_t r = bitreader.Read(P);

    return (q << P) + r;
}

#endif // SYSCOIN_UTIL_GOLOMBRICE_H
