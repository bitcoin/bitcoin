// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <crypto/hex_base.h>
#include <index/ordered_varint.h>
#include <streams.h>

#include <cstddef>
#include <cstdint>
#include <ios>
#include <limits>
#include <string>
#include <utility>
#include <vector>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(ordered_varint_tests)

static std::vector<std::byte> EncodeOrderedVarInt(uint64_t value)
{
    DataStream stream;
    stream << Using<OrderedVarIntFormatter>(value);
    return {stream.begin(), stream.end()};
}

static uint64_t DecodeOrderedVarInt(const std::vector<std::byte>& bytes)
{
    DataStream stream{bytes};
    uint64_t value{0};
    stream >> Using<OrderedVarIntFormatter>(value);
    BOOST_CHECK(stream.empty());
    return value;
}

BOOST_AUTO_TEST_CASE(ordered_varint_encoding_roundtrip_and_order)
{
    const struct {
        uint64_t value;
        std::string encoded;
    } cases[]{
        {0, "00"},
        {1, "01"},
        {126, "7e"},
        {127, "7f"},
        {128, "8000"},
        {255, "807f"},
        {16511, "bfff"},
        {16512, "c00000"},
        {16513, "c00001"},
        {2'000'000, "de4400"},
        {2'113'663, "dfffff"},
        {2'113'664, "e0000000"},
        {270'549'119, "efffffff"},
        {270'549'120, "f000000000"},
        {4'000'000'000ULL, "f0de4ae780"},
        {std::numeric_limits<uint32_t>::max(), "f0efdfbf7f"},
        {OrderedVarIntFormatter::MAX_5, "f7ffffffff"},
    };
    std::vector<std::byte> previous;
    for (const auto& [value, encoded] : cases) {
        auto bytes{EncodeOrderedVarInt(value)};
        BOOST_CHECK_EQUAL(HexStr(bytes), encoded);
        BOOST_CHECK_EQUAL(DecodeOrderedVarInt(bytes), value);
        BOOST_CHECK(previous < bytes);
        previous = std::move(bytes);
    }
}

BOOST_AUTO_TEST_CASE(ordered_varint_invalid_encodings)
{
    // Exercise every truncation length for each supported encoding size.
    for (const uint64_t value : {uint64_t{0}, uint64_t{128}, uint64_t{16512}, uint64_t{2'113'664}, OrderedVarIntFormatter::MAX_5}) {
        const auto bytes{EncodeOrderedVarInt(value)};
        for (size_t size{0}; size < bytes.size(); ++size) {
            BOOST_CHECK_THROW(DecodeOrderedVarInt({bytes.begin(), bytes.begin() + size}), std::ios_base::failure);
        }
    }
    for (uint32_t prefix{0xf8}; prefix <= 0xff; ++prefix) {
        BOOST_CHECK_THROW(DecodeOrderedVarInt({static_cast<std::byte>(prefix), std::byte{0}, std::byte{0}, std::byte{0}, std::byte{0}}), std::ios_base::failure);
    }
    BOOST_CHECK_THROW(EncodeOrderedVarInt(OrderedVarIntFormatter::MAX_5 + 1), std::ios_base::failure);
    BOOST_CHECK_THROW(EncodeOrderedVarInt(std::numeric_limits<uint64_t>::max()), std::ios_base::failure);

    uint32_t decoded{0};
    DataStream maximum{EncodeOrderedVarInt(std::numeric_limits<uint32_t>::max())};
    maximum >> Using<OrderedVarIntFormatter>(decoded);
    BOOST_CHECK_EQUAL(decoded, std::numeric_limits<uint32_t>::max());
    DataStream overflow{EncodeOrderedVarInt(uint64_t{std::numeric_limits<uint32_t>::max()} + 1)};
    BOOST_CHECK_THROW(overflow >> Using<OrderedVarIntFormatter>(decoded), std::ios_base::failure);
    BOOST_CHECK_EQUAL(decoded, std::numeric_limits<uint32_t>::max());
}

BOOST_AUTO_TEST_SUITE_END()
