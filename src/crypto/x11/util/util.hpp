// Copyright (c) 2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CRYPTO_X11_UTIL_UTIL_HPP
#define BITCOIN_CRYPTO_X11_UTIL_UTIL_HPP

#include <cstdint>

namespace sapphire {
namespace util {
constexpr inline uint32_t pack_le(uint8_t b3, uint8_t b2, uint8_t b1, uint8_t b0) noexcept
{
    return (static_cast<uint32_t>(b3) << 24) |
           (static_cast<uint32_t>(b2) << 16) |
           (static_cast<uint32_t>(b1) <<  8) |
           (static_cast<uint32_t>(b0));
}
} // namespace util
} // namespace sapphire

#endif // BITCOIN_CRYPTO_X11_UTIL_UTIL_HPP
