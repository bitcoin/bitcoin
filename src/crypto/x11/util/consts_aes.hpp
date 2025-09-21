// Copyright (c) 2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CRYPTO_X11_UTIL_CONSTS_AES_HPP
#define BITCOIN_CRYPTO_X11_UTIL_CONSTS_AES_HPP

#include <crypto/x11/util/util.hpp>

#include <array>
#include <bit>
#include <cstddef>

namespace sapphire {
namespace util {
constexpr inline uint8_t gf8_mul2(uint8_t x) noexcept {
    return (x << 1) ^ ((x & 0x80) ? 0x1b : 0x00);
}

constexpr auto gf8_log = []() constexpr {
    std::array<uint8_t, 256> ret{};
    uint8_t x{1};
    for (size_t idx{0}; idx < ret.size(); ++idx) {
        ret[x] = static_cast<uint8_t>(idx);
        x = (x ^ gf8_mul2(x)) & 0xff;
    }
    return ret;
}();

constexpr auto gf8_pow = []() constexpr {
    std::array<uint8_t, 256> ret{};
    uint8_t x{1};
    for (size_t idx{0}; idx < ret.size(); ++idx) {
        ret[idx] = x;
        x = (x ^ gf8_mul2(x)) & 0xff;
    }
    return ret;
}();

constexpr inline uint8_t gf8_mul(uint8_t x, uint8_t y) noexcept {
    if (x != 0 && y != 0) {
        return gf8_pow[(gf8_log[x] + gf8_log[y]) % 255];
    }
    return 0;
}
} // namespace util

namespace consts {
constexpr auto aes_sbox_f = []() constexpr {
    std::array<uint8_t, 256> ret{};
    uint8_t x{0}, y{0};
    ret[0x00] = 0x63;
    for (size_t idx{1}; idx < ret.size(); ++idx) {
        x = util::gf8_pow[255 - util::gf8_log[idx]];
        y  = x; y = std::rotl(y, 1);
        x ^= y; y = std::rotl(y, 1);
        x ^= y; y = std::rotl(y, 1);
        x ^= y; y = std::rotl(y, 1);
        x ^= y ^ 0x63;
        ret[idx] = x;
    }
    return ret;
}();

constexpr auto aes_tbox_le = []() constexpr {
    std::array<std::array<uint32_t, 256>, 4> ret{};
    for (size_t idx{0}; idx < 256; ++idx) {
        const uint8_t byte{aes_sbox_f[idx]};
        uint32_t word{util::pack_le(util::gf8_mul(byte, 3), byte, byte, util::gf8_mul(byte, 2))};
        for (size_t jdx{0}; jdx < 4; ++jdx) {
            ret[jdx][idx] = word;
            word = std::rotl(word, 8);
        }
    }
    return ret;
}();

static_assert(aes_tbox_le[0][  0] == 0xa56363c6 &&
              aes_tbox_le[0][255] == 0x3a16162c &&
              aes_tbox_le[3][  0] == 0xc6a56363 &&
              aes_tbox_le[3][255] == 0x2c3a1616, "Bad little-endian AES transform table");
} // namespace consts
} // namespace sapphire

#endif // BITCOIN_CRYPTO_X11_UTIL_CONSTS_AES_HPP
