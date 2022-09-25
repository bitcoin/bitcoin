// Copyright (c) 2022 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <blsct/arith/range_proof/config.h>

size_t Config::GetFirstPowerOf2GreaterOrEqTo(const size_t& n)
{
    size_t i = 1;
    while (i < n) {
        i *= 2;
    }
    return i;
}