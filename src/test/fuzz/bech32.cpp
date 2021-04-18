// Copyright (c) 2019 The Widecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bech32.h>
#include <test/fuzz/fuzz.h>
#include <test/util/str.h>
#include <util/strencodings.h>

#include <cassert>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

void test_one_input(const std::vector<uint8_t>& buffer)
{
    const std::string random_string(buffer.begin(), buffer.end());
    const std::pair<std::string, std::vector<uint8_t>> r1 = bech32::Decode(random_string);
    if (r1.first.empty()) {
        assert(r1.second.empty());
    } else {
        const std::string& hrp = r1.first;
        const std::vector<uint8_t>& data = r1.second;
        const std::string reencoded = bech32::Encode(hrp, data);
        assert(CaseInsensitiveEqual(random_string, reencoded));
    }

    std::vector<unsigned char> input;
    ConvertBits<8, 5, true>([&](unsigned char c) { input.push_back(c); }, buffer.begin(), buffer.end());
    const std::string encoded = bech32::Encode("bc", input);
    assert(!encoded.empty());

    const std::pair<std::string, std::vector<uint8_t>> r2 = bech32::Decode(encoded);
    if (r2.first.empty()) {
        assert(r2.second.empty());
    } else {
        const std::string& hrp = r2.first;
        const std::vector<uint8_t>& data = r2.second;
        assert(hrp == "bc");
        assert(data == input);
    }
}
