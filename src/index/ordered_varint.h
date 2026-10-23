// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_INDEX_ORDERED_VARINT_H
#define BITCOIN_INDEX_ORDERED_VARINT_H

#include <serialize.h>

#include <cstdint>
#include <ios>
#include <limits>
#include <type_traits>

/**
 * Unsigned integer encoding whose lexicographic byte order matches numeric order.
 *
 * Length is stored in the leading bits of the first byte (UTF-8 style), so
 * every n-byte encoding sorts before every (n+1)-byte encoding. Payload bits
 * are big-endian after subtracting the ranges covered by shorter encodings.
 * Supports up to five bytes, sufficient for any uint32_t. Size cutovers match
 * Bitcoin Core VARINT:
 *
 *   1 byte: 0 .. 127
 *   2 byte: 128 .. 16,511
 *   3 byte: 16,512 .. 2,113,663
 *   4 byte: 2,113,664 .. 270,549,119
 *   5 byte: 270,549,120 .. 34,630,287,487
 *
 * Unlike VARINT, this can be used as a LevelDB key field for range seeks.
 */
struct OrderedVarIntFormatter {
    static constexpr uint64_t MAX_1{0x7F};
    static constexpr uint64_t MAX_2{0x407F};
    static constexpr uint64_t MAX_3{0x20407F};
    static constexpr uint64_t MAX_4{0x1020407F};
    static constexpr uint64_t MAX_5{MAX_4 + (uint64_t{1} << 35)};

    template <typename Stream>
    static void Write(Stream& s, uint64_t n)
    {
        if (n > MAX_5) {
            throw std::ios_base::failure("OrderedVarInt value too large");
        }
        uint32_t length{1};
        uint8_t prefix{0};
        // Subtract the ranges represented by shorter encodings.
        for (uint64_t range{128}; n >= range; range <<= 7) {
            n -= range;
            ++length;
            prefix = (prefix >> 1) | 0x80;
        }
        ser_writedata8(s, prefix | (n >> (8 * (length - 1))));
        while (--length > 0) {
            ser_writedata8(s, static_cast<uint8_t>(n >> (8 * (length - 1))));
        }
    }

    template <typename Stream>
    static uint64_t Read(Stream& s)
    {
        const uint8_t first{ser_readdata8(s)};
        if (first >= 0xF8) {
            throw std::ios_base::failure("Invalid OrderedVarInt encoding");
        }
        uint32_t length{1};
        uint64_t offset{0};
        uint8_t mask{0x80};
        // Leading one bits count additional bytes and the shorter ranges to restore.
        for (uint64_t range{128}; first & mask; range <<= 7) {
            offset += range;
            ++length;
            mask >>= 1;
        }
        uint64_t n{static_cast<uint64_t>(first & (mask - 1))};
        for (uint32_t i{1}; i < length; ++i) {
            n = (n << 8) | ser_readdata8(s);
        }
        return offset + n;
    }

    template <typename Stream, typename I>
    void Ser(Stream& s, I v)
    {
        static_assert(std::is_unsigned_v<I>, "OrderedVarInt requires an unsigned type");
        Write(s, v);
    }

    template <typename Stream, typename I>
    void Unser(Stream& s, I& v)
    {
        static_assert(std::is_unsigned_v<I>, "OrderedVarInt requires an unsigned type");
        const uint64_t n{Read(s)};
        if (n > std::numeric_limits<I>::max()) {
            throw std::ios_base::failure("OrderedVarInt exceeds limit of type");
        }
        v = static_cast<I>(n);
    }
};

#endif // BITCOIN_INDEX_ORDERED_VARINT_H
