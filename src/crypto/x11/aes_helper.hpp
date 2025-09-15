/* $Id: aes_helper.c 220 2010-06-09 09:21:50Z tp $ */
/*
 * AES tables. This file is not meant to be compiled by itself; it
 * is included by some hash function implementations. It contains
 * the precomputed tables and helper macros for evaluating an AES
 * round, optionally with a final XOR with a subkey.
 *
 * By default, this file defines the tables and macros for little-endian
 * processing (i.e. it is assumed that the input bytes have been read
 * from memory and assembled with the little-endian convention). If
 * the 'AES_BIG_ENDIAN' macro is defined (to a non-zero integer value)
 * when this file is included, then the tables and macros for big-endian
 * processing are defined instead. The big-endian tables and macros have
 * names distinct from the little-endian tables and macros, hence it is
 * possible to have both simultaneously, by including this file twice
 * (with and without the AES_BIG_ENDIAN macro).
 *
 * ==========================(LICENSE BEGIN)============================
 *
 * Copyright (c) 2007-2010  Projet RNRT SAPHIR
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * ===========================(LICENSE END)=============================
 *
 * @author   Thomas Pornin <thomas.pornin@cryptolog.com>
 */

#ifndef SPH_AES_HELPER_HPP__
#define SPH_AES_HELPER_HPP__

#include <attributes.h>

#include "sph_types.h"

#include <array>
#include <bit>

namespace impl {
constexpr inline uint32_t pack_le(uint8_t b3, uint8_t b2, uint8_t b1, uint8_t b0) noexcept
{
    return (static_cast<uint32_t>(b3) << 24) |
           (static_cast<uint32_t>(b2) << 16) |
           (static_cast<uint32_t>(b1) <<  8) |
           (static_cast<uint32_t>(b0));
}

constexpr inline uint8_t gf8_mul2(uint8_t x) noexcept { return (x << 1) ^ ((x & 0x80) ? 0x1b : 0x00); }

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

constexpr auto aes_sbox_f = []() constexpr {
    std::array<uint8_t, 256> ret{};
    uint8_t x{0}, y{0};
    ret[0x00] = 0x63;
    for (size_t idx{1}; idx < ret.size(); ++idx) {
        x = gf8_pow[255 - gf8_log[idx]];
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
        uint32_t word{pack_le(gf8_mul(byte, 3), byte, byte, gf8_mul(byte, 2))};
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
} // namespace impl

void ALWAYS_INLINE AES_ROUND_LE(sph_u32 x0, sph_u32 x1, sph_u32 x2, sph_u32 x3,
								sph_u32 k0, sph_u32 k1, sph_u32 k2, sph_u32 k3,
								sph_u32& y0, sph_u32& y1, sph_u32& y2, sph_u32& y3)
{
	using namespace impl;
    y0 = aes_tbox_le[0][(x0) & 0xff] ^ aes_tbox_le[1][((x1) >> 8) & 0xff] ^ aes_tbox_le[2][((x2) >> 16) & 0xff] ^ aes_tbox_le[3][((x3) >> 24) & 0xff] ^ (k0);
    y1 = aes_tbox_le[0][(x1) & 0xff] ^ aes_tbox_le[1][((x2) >> 8) & 0xff] ^ aes_tbox_le[2][((x3) >> 16) & 0xff] ^ aes_tbox_le[3][((x0) >> 24) & 0xff] ^ (k1);
    y2 = aes_tbox_le[0][(x2) & 0xff] ^ aes_tbox_le[1][((x3) >> 8) & 0xff] ^ aes_tbox_le[2][((x0) >> 16) & 0xff] ^ aes_tbox_le[3][((x1) >> 24) & 0xff] ^ (k2);
    y3 = aes_tbox_le[0][(x3) & 0xff] ^ aes_tbox_le[1][((x0) >> 8) & 0xff] ^ aes_tbox_le[2][((x1) >> 16) & 0xff] ^ aes_tbox_le[3][((x2) >> 24) & 0xff] ^ (k3);
}

void ALWAYS_INLINE AES_ROUND_NOKEY_LE(sph_u32 x0, sph_u32 x1, sph_u32 x2, sph_u32 x3,
									  sph_u32& y0, sph_u32& y1, sph_u32& y2, sph_u32& y3)
{
	AES_ROUND_LE(x0, x1, x2, x3, /*k0=*/0, /*k1=*/0, /*k2=*/0, /*k3=*/0, y0, y1, y2, y3);
}

#endif // SPH_AES_HELPER_HPP__
