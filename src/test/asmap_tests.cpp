// Copyright (c) 2024-2025 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <util/asmap.h>

#include <boost/test/unit_test.hpp>

#include <cstdint>
#include <initializer_list>
#include <vector>

namespace {

enum class Instruction : uint32_t
{
    RETURN = 0,
    JUMP = 1,
    MATCH = 2,
    DEFAULT = 3,
};

const std::vector<uint8_t> TYPE_BIT_SIZES{0, 0, 1};
const std::vector<uint8_t> ASN_BIT_SIZES{15, 16, 17, 18, 19, 20, 21, 22, 23, 24};
const std::vector<uint8_t> MATCH_BIT_SIZES{1, 2, 3, 4, 5, 6, 7, 8};

void EncodeBits(std::vector<bool>& out, uint32_t value, uint32_t minval, const std::vector<uint8_t>& bit_sizes)
{
    BOOST_REQUIRE(value >= minval);
    uint32_t current = minval;
    for (size_t i = 0; i < bit_sizes.size(); ++i) {
        const int size = static_cast<int>(bit_sizes[i]);
        if (i + 1 == bit_sizes.size()) {
            const uint32_t diff = value - current;
            const uint32_t max_value = size == 0 ? 1U : (1U << size);
            BOOST_REQUIRE(diff < max_value);
            for (int bit = size - 1; bit >= 0; --bit) {
                out.push_back((diff >> bit) & 1U);
            }
            return;
        }
        const uint32_t range = 1U << size;
        const uint32_t threshold = current + range;
        if (value < threshold) {
            out.push_back(false);
            const uint32_t diff = value - current;
            for (int bit = size - 1; bit >= 0; --bit) {
                out.push_back((diff >> bit) & 1U);
            }
            return;
        }
        out.push_back(true);
        current += range;
    }
    BOOST_FAIL("Failed to encode value with provided prefix table");
}

void EncodeType(std::vector<bool>& out, Instruction instruction)
{
    EncodeBits(out, static_cast<uint32_t>(instruction), 0, TYPE_BIT_SIZES);
}

void EncodeASN(std::vector<bool>& out, uint32_t asn)
{
    EncodeBits(out, asn, 1, ASN_BIT_SIZES);
}

void EncodeMatch(std::vector<bool>& out, uint32_t match)
{
    EncodeBits(out, match, 2, MATCH_BIT_SIZES);
}

uint32_t BuildMatchValue(std::initializer_list<int> pattern_bits)
{
    const size_t length = pattern_bits.size();
    BOOST_REQUIRE(length == 0 || length <= 8);
    uint32_t pattern = 0;
    for (int bit : pattern_bits) {
        BOOST_REQUIRE(bit == 0 || bit == 1);
        pattern = (pattern << 1) | static_cast<uint32_t>(bit);
    }
    return (1U << static_cast<unsigned>(length)) | pattern;
}

void PadToByteBoundary(std::vector<bool>& bits)
{
    while (bits.size() % 8 != 0) bits.push_back(false);
}

std::vector<bool> BuildReturnProgram(uint32_t asn)
{
    std::vector<bool> program;
    EncodeType(program, Instruction::RETURN);
    EncodeASN(program, asn);
    PadToByteBoundary(program);
    return program;
}

std::vector<bool> BuildDefaultMatchProgram(uint32_t default_asn, std::initializer_list<int> pattern_bits, uint32_t matched_asn)
{
    std::vector<bool> program;
    EncodeType(program, Instruction::DEFAULT);
    EncodeASN(program, default_asn);
    EncodeType(program, Instruction::MATCH);
    EncodeMatch(program, BuildMatchValue(pattern_bits));
    EncodeType(program, Instruction::RETURN);
    EncodeASN(program, matched_asn);
    PadToByteBoundary(program);
    return program;
}

} // namespace

BOOST_AUTO_TEST_SUITE(asmap_tests)

BOOST_AUTO_TEST_CASE(asmap_interpret_respects_default_and_match)
{
    auto asmap = BuildDefaultMatchProgram(/*default_asn=*/9, {1, 0}, /*matched_asn=*/42);
    BOOST_REQUIRE(SanityCheckASMap(asmap, 128));

    std::vector<bool> matching_ip(128);
    matching_ip[0] = true;
    matching_ip[1] = false;
    BOOST_CHECK_EQUAL(Interpret(asmap, matching_ip), 42U);

    std::vector<bool> non_matching_ip(128);
    non_matching_ip[0] = true;
    non_matching_ip[1] = true;
    BOOST_CHECK_EQUAL(Interpret(asmap, non_matching_ip), 9U);
}

BOOST_AUTO_TEST_CASE(asmap_missing_return_is_invalid)
{
    std::vector<bool> asmap;
    EncodeType(asmap, Instruction::DEFAULT);
    EncodeASN(asmap, 1);
    PadToByteBoundary(asmap);
    BOOST_CHECK(!SanityCheckASMap(asmap, 128));
}

BOOST_AUTO_TEST_CASE(asmap_default_directly_followed_by_return_is_invalid)
{
    std::vector<bool> asmap;
    EncodeType(asmap, Instruction::DEFAULT);
    EncodeASN(asmap, 1);
    EncodeType(asmap, Instruction::RETURN);
    EncodeASN(asmap, 2);
    PadToByteBoundary(asmap);
    BOOST_CHECK(!SanityCheckASMap(asmap, 128));
}

BOOST_AUTO_TEST_CASE(asmap_requires_sufficient_address_bits)
{
    auto asmap = BuildDefaultMatchProgram(/*default_asn=*/5, {0, 1, 0}, /*matched_asn=*/7);
    BOOST_REQUIRE(SanityCheckASMap(asmap, 128));
    BOOST_CHECK(!SanityCheckASMap(asmap, 2));
}

BOOST_AUTO_TEST_CASE(asmap_excessive_padding_is_rejected)
{
    auto asmap = BuildReturnProgram(/*asn=*/13);
    asmap.insert(asmap.end(), 8, false);
    BOOST_CHECK(!SanityCheckASMap(asmap, 128));
}

BOOST_AUTO_TEST_SUITE_END()

