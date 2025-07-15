// Copyright (c) 2025-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_OBFUSCATION_H
#define BITCOIN_UTIL_OBFUSCATION_H

#include <cstdint>

class Obfuscation
{
public:
    static constexpr size_t KEY_SIZE{sizeof(uint64_t)};
};

#endif // BITCOIN_UTIL_OBFUSCATION_H
