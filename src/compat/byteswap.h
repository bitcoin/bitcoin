// Copyright (c) 2014-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_COMPAT_BYTESWAP_H
#define BITCOIN_COMPAT_BYTESWAP_H

#include <bit>
#include <cstdint>

inline constexpr uint16_t internal_bswap_16(uint16_t x)
{
    return std::byteswap(x);
}

inline constexpr uint32_t internal_bswap_32(uint32_t x)
{
    return std::byteswap(x);
}

inline constexpr uint64_t internal_bswap_64(uint64_t x)
{
    return std::byteswap(x);
}

#endif // BITCOIN_COMPAT_BYTESWAP_H
