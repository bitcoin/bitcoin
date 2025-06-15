// Copyright (c) 2009-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <crypto/hex_base.h>

#include <array>
#include <cstring>
#include <string>

namespace {

using ByteAsHex = std::array<char, 2>;

constexpr std::array<ByteAsHex, 256> CreateByteToHexMap()
{
    constexpr char hexmap[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

    std::array<ByteAsHex, 256> byte_to_hex{};
    for (size_t i = 0; i < byte_to_hex.size(); ++i) {
        byte_to_hex[i][0] = hexmap[i >> 4];
        byte_to_hex[i][1] = hexmap[i & 15];
    }
    return byte_to_hex;
}

} // namespace

std::string HexStr(const std::span<const uint8_t> s)
{
    std::string rv(s.size() * 2, '\0');
    static constexpr auto byte_to_hex = CreateByteToHexMap();
    static_assert(sizeof(byte_to_hex) == 512);

    char* it = rv.data();
    for (uint8_t v : s) {
        std::memcpy(it, byte_to_hex[v].data(), 2);
        it += 2;
    }

    assert(it == rv.data() + rv.size());
    return rv;
}

constexpr std::array<signed char, 256> CreateHexDigitMap()
{
    std::array<signed char, 256> map{};
    map.fill(-1);
    for (int i = 0; i < 10; ++i)
        map['0' + i] = i;
    for (int i = 0; i < 6; ++i) {
        map['a' + i] = 10 + i;
        map['A' + i] = 10 + i;
    }
    return map;
}

static constexpr auto p_util_hexdigit = CreateHexDigitMap();
static_assert(p_util_hexdigit.size() == 256);

signed char HexDigit(char c)
{
    return p_util_hexdigit[static_cast<unsigned char>(c)];
}
