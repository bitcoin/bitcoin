// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UINT256_H
#define BITCOIN_UINT256_H

#include <crypto/common.h>
#include <span.h>
#include <util/strencodings.h>
#include <util/string.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <optional>
#include <string>
#include <string_view>

/** Template base class for fixed-sized opaque blobs. */
template<unsigned int BITS>
class base_blob
{
protected:
    static constexpr int WIDTH = BITS / 8;
    static_assert(BITS % 8 == 0, "base_blob currently only supports whole bytes.");
    std::array<uint8_t, WIDTH> m_data;
    static_assert(WIDTH == sizeof(m_data), "Sanity check");

public:
    /* construct 0 value by default */
    constexpr base_blob() : m_data() {}

    /* constructor for constants between 1 and 255 */
    constexpr explicit base_blob(uint8_t v) : m_data{v} {}

    constexpr explicit base_blob(Span<const unsigned char> vch)
    {
        assert(vch.size() == WIDTH);
        std::copy(vch.begin(), vch.end(), m_data.begin());
    }

    consteval explicit base_blob(std::string_view hex_str);

    constexpr bool IsNull() const
    {
        return std::all_of(m_data.begin(), m_data.end(), [](uint8_t val) {
            return val == 0;
        });
    }

    constexpr void SetNull()
    {
        std::fill(m_data.begin(), m_data.end(), 0);
    }

    /** Lexicographic ordering
     * @note Does NOT match the ordering on the corresponding \ref
     *       base_uint::CompareTo, which starts comparing from the end.
     */
    constexpr int Compare(const base_blob& other) const { return std::memcmp(m_data.data(), other.m_data.data(), WIDTH); }

    friend constexpr bool operator==(const base_blob& a, const base_blob& b) { return a.Compare(b) == 0; }
    friend constexpr bool operator!=(const base_blob& a, const base_blob& b) { return a.Compare(b) != 0; }
    friend constexpr bool operator<(const base_blob& a, const base_blob& b) { return a.Compare(b) < 0; }

    /** @name Hex representation
     *
     * The reverse-byte hex representation is a convenient way to view the blob
     * as a number, because it is consistent with the way the base_uint class
     * converts blobs to numbers.
     *
     * @note base_uint treats the blob as an array of bytes with the numerically
     * least significant byte first and the most significant byte last. Because
     * numbers are typically written with the most significant digit first and
     * the least significant digit last, the reverse hex display of the blob
     * corresponds to the same numeric value that base_uint interprets from the
     * blob.
     * @{*/
    std::string GetHex() const;
    /** Unlike FromHex this accepts any invalid input, thus it is fragile and deprecated!
     *
     * - Hex numbers that don't specify enough bytes to fill the internal array
     *   will be treated as setting the beginning of it, which corresponds to
     *   the least significant bytes when converted to base_uint.
     *
     * - Hex numbers specifying too many bytes will have the numerically most
     *   significant bytes (the beginning of the string) narrowed away.
     *
     * - An odd count of hex digits will result in the high bits of the leftmost
     *   byte being zero.
     *   "0x123" => {0x23, 0x1, 0x0, ..., 0x0}
     */
    void SetHexDeprecated(std::string_view str);
    std::string ToString() const;
    /**@}*/

    constexpr const unsigned char* data() const { return m_data.data(); }
    constexpr unsigned char* data() { return m_data.data(); }

    constexpr unsigned char* begin() { return m_data.data(); }
    constexpr unsigned char* end() { return m_data.data() + WIDTH; }

    constexpr const unsigned char* begin() const { return m_data.data(); }
    constexpr const unsigned char* end() const { return m_data.data() + WIDTH; }

    static constexpr unsigned int size() { return WIDTH; }

    constexpr uint64_t GetUint64(int pos) const { return ReadLE64(m_data.data() + pos * 8); }

    template<typename Stream>
    void Serialize(Stream& s) const
    {
        s << Span(m_data);
    }

    template<typename Stream>
    void Unserialize(Stream& s)
    {
        s.read(MakeWritableByteSpan(m_data));
    }
};

template <unsigned int BITS>
consteval base_blob<BITS>::base_blob(std::string_view hex_str)
{
    if (hex_str.length() != m_data.size() * 2) throw "Hex string must fit exactly";
    auto str_it = hex_str.rbegin();
    for (auto& elem : m_data) {
        auto lo = util::ConstevalHexDigit(*(str_it++));
        elem = (util::ConstevalHexDigit(*(str_it++)) << 4) | lo;
    }
}

namespace detail {
/**
 * Writes the hex string (in reverse byte order) into a new uintN_t object
 * and only returns a value iff all of the checks pass:
 *   - Input length is uintN_t::size()*2
 *   - All characters are hex
 */
template <class uintN_t>
std::optional<uintN_t> FromHex(std::string_view str)
{
    if (uintN_t::size() * 2 != str.size() || !IsHex(str)) return std::nullopt;
    uintN_t rv;
    rv.SetHexDeprecated(str);
    return rv;
}
/**
 * @brief Like FromHex(std::string_view str), but allows an "0x" prefix
 *        and pads the input with leading zeroes if it is shorter than
 *        the expected length of uintN_t::size()*2.
 *
 *        Designed to be used when dealing with user input.
 */
template <class uintN_t>
std::optional<uintN_t> FromUserHex(std::string_view input)
{
    input = util::RemovePrefixView(input, "0x");
    constexpr auto expected_size{uintN_t::size() * 2};
    if (input.size() < expected_size) {
        auto padded = std::string(expected_size, '0');
        std::copy(input.begin(), input.end(), padded.begin() + expected_size - input.size());
        return FromHex<uintN_t>(padded);
    }
    return FromHex<uintN_t>(input);
}
} // namespace detail

/** 160-bit opaque blob.
 * @note This type is called uint160 for historical reasons only. It is an opaque
 * blob of 160 bits and has no integer operations.
 */
class uint160 : public base_blob<160> {
public:
    static std::optional<uint160> FromHex(std::string_view str) { return detail::FromHex<uint160>(str); }
    constexpr uint160() = default;
    constexpr explicit uint160(Span<const unsigned char> vch) : base_blob<160>(vch) {}
};

/** 256-bit opaque blob.
 * @note This type is called uint256 for historical reasons only. It is an
 * opaque blob of 256 bits and has no integer operations. Use arith_uint256 if
 * those are required.
 */
class uint256 : public base_blob<256> {
public:
    static std::optional<uint256> FromHex(std::string_view str) { return detail::FromHex<uint256>(str); }
    static std::optional<uint256> FromUserHex(std::string_view str) { return detail::FromUserHex<uint256>(str); }
    constexpr uint256() = default;
    consteval explicit uint256(std::string_view hex_str) : base_blob<256>(hex_str) {}
    constexpr explicit uint256(uint8_t v) : base_blob<256>(v) {}
    constexpr explicit uint256(Span<const unsigned char> vch) : base_blob<256>(vch) {}
    static const uint256 ZERO;
    static const uint256 ONE;
};

#endif // BITCOIN_UINT256_H
