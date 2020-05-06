// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <util/asmap.h>
#include <test/fuzz/fuzz.h>

#include <algorithm>
#include <cstdint>
#include <functional>
#include <vector>
#include <stdio.h>
#include <unordered_map>
#include <set>

#include <assert.h>

namespace {

/** Extend every prefix with a number of patterns for testing. */
const std::vector<uint32_t> TEST_PATTERNS = {0, 0xffffffff, 0x55555555, 0xaaaaaaa, 0x2461611a, 0x66eac6e1, 0x943eeccb, 0xc2889f4d};

/** Bytes 0x02..0xFF at the end of a 0x00/0x01 sequence determines the ASN for
 *  that prefix. Those bytes are mapped to an geometric progressing of valid
 *  ASN values to give a wide range.
 */
const uint32_t TEST_ASNS[254] = {
    1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 22,
    23, 25, 26, 28, 29, 31, 33, 35, 38, 40, 43, 45, 48, 51, 54, 58, 61, 65, 69,
    74, 78, 83, 89, 94, 100, 107, 113, 120, 128, 136, 145, 154, 163, 174, 185,
    196, 209, 222, 236, 251, 267, 283, 301, 320, 340, 362, 385, 409, 435, 462,
    491, 522, 555, 590, 628, 667, 709, 754, 802, 852, 906, 963, 1024, 1088,
    1157, 1230, 1307, 1390, 1478, 1571, 1670, 1775, 1887, 2006, 2133, 2267,
    2410, 2562, 2724, 2895, 3078, 3272, 3478, 3698, 3931, 4179, 4442, 4722,
    5020, 5337, 5673, 6031, 6412, 6816, 7246, 7703, 8189, 8705, 9254, 9838,
    10458, 11117, 11819, 12564, 13356, 14199, 15094, 16046, 17058, 18134,
    19277, 20493, 21785, 23159, 24620, 26172, 27823, 29578, 31443, 33426,
    35534, 37775, 40157, 42689, 45382, 48244, 51286, 54520, 57959, 61614,
    65499, 69630, 74021, 78689, 83652, 88927, 94536, 100498, 106835, 113573,
    120735, 128349, 136444, 145049, 154196, 163920, 174258, 185247, 196930,
    209349, 222552, 236587, 251507, 267369, 284230, 302155, 321210, 341467,
    363002, 385894, 410231, 436102, 463604, 492841, 523922, 556963, 592088,
    629428, 669122, 711320, 756180, 803868, 854563, 908456, 965748, 1026652,
    1091398, 1160226, 1233396, 1311179, 1393869, 1481772, 1575220, 1674561,
    1780166, 1892432, 2011778, 2138650, 2273523, 2416902, 2569323, 2731356,
    2903609, 3086724, 3281387, 3488327, 3708317, 3942181, 4190793, 4455084,
    4736042, 5034719, 5352232, 5689769, 6048592, 6430045, 6835554, 7266636,
    7724904, 8212072, 8729964, 9280516, 9865789, 10487972, 11149392, 11852525,
    12600001, 13394616, 14239343, 15137342, 16091974, 17106809, 18185644,
    19332516, 20551715, 21847802, 23225627, 24690343, 26247432, 27902718,
    29662394, 31533043, 33521664
};

using elem = std::pair<std::vector<bool>, uint32_t>;
using elem_ref = std::reference_wrapper<const elem>;

void UpdateExpect(std::unordered_map<std::vector<bool>, uint32_t>& expect, const std::vector<bool>& key, uint32_t asn)
{
    std::vector<bool> expanded_key;
    for (uint32_t pattern : TEST_PATTERNS) {
        expanded_key = key;
        expanded_key.resize(32);
        for (size_t pos = key.size(); pos < 32; ++pos) expanded_key[pos] = (pattern >> pos) & 1;
        expect[std::move(expanded_key)] = asn;
    }
}

void TestASMap(std::vector<elem> input, const std::vector<elem_ref>& input_sorted, bool approx)
{
    // Iterate over the input in order from short to long, and build up
    // a map of expected inputs to outputs. By going from short to long,
    // the later inputs will correctly overwrite the mappings created by
    // their prefixes.
    std::unordered_map<std::vector<bool>, uint32_t> expect;
    if (!approx) UpdateExpect(expect, {}, 0); // When !approx, everything not explicitly overridden must map to 0
    for (elem_ref item : input_sorted) {
        UpdateExpect(expect, item.get().first, item.get().second);
    }

    // Construct an asmap.
    auto asmap = EncodeASMap(std::move(input), approx);
    // It must sanity check.
    assert(SanityCheckASMap(asmap, 32));
    // All expected lookups must work.
    for (const auto& item : expect) {
        assert(Interpret(asmap, item.first) == item.second);
    }

    // Check that decoding and re-encoding yields an identical results.
    // This guarantees that DecodeASMap does not lose any information.
    auto decoded_asmap = DecodeASMap(asmap, 32);
    auto recoded_asmap = EncodeASMap(std::move(decoded_asmap), approx);
    // All expected lookups must work.
    for (const auto& item : expect) {
        assert(Interpret(recoded_asmap, item.first) == item.second);
    }
}

}

void test_one_input(const std::vector<uint8_t>& buffer)
{
    // Input format: ([0-32 0/1 bytes for prefix bits of netmask] [1 byte 2-255 for ASN])*

    // First do a sanity check on the input (so we quickly let the fuzzer know this is uninteresting)
    if (buffer.size() == 0) return;
    auto it = buffer.begin();
    while (it != buffer.end()) {
        auto mask_begin = it;
        while (it != buffer.end() && (*it & 0xfe) == 0) ++it;
        if (it - mask_begin > 32) return; // Netmask too short or too long
        if (it == buffer.end()) return; // Missing ASN
        ++it;
    }

    // Parse the input into a format that EncodeASMap understands
    std::vector<elem> input;
    std::set<std::vector<bool>> already;
    it = buffer.begin();
    while (it != buffer.end()) {
        auto mask_begin = it;
        while ((*it & 0xfe) == 0) ++it;
        auto mask = std::vector<bool>(mask_begin, it);
        if (!already.emplace(mask).second) return; // Duplicate netmask in input
        input.emplace_back(std::move(mask), TEST_ASNS[(*it) - 2]);
        ++it;
    }

    // Testing requires a sorted version of the inputs as well.
    // Use a vector of reference_wrapper to avoid copying the inputs.
    std::vector<elem_ref> input_sorted(input.begin(), input.end());
    std::sort(input_sorted.begin(), input_sorted.end(), [](elem_ref a, elem_ref b) { return a.get().first.size() < b.get().first.size(); });

    // Test with approx = true
    TestASMap(input, input_sorted, true);

    // Test with approx = false
    TestASMap(std::move(input), input_sorted, false);
}
