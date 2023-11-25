// Copyright (c) 2019-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bech32.h>
#include <test/fuzz/fuzz.h>
#include <test/util/str.h>
#include <util/strencodings.h>

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <random>
#include <string>
#include <utility>
#include <vector>

FUZZ_TARGET(bech32)
{
    // create a buf of the size valid for decoding
    std::vector<uint8_t> buf(buffer.begin(), buffer.end());

    if (buf.size() > 154) {
        buf.resize(154);
    } else if (buf.size() < 154) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<uint8_t> dist(0, 255);

        while (buf.size() < 154) {
            buf.push_back(dist(gen));
        }
    }

    const std::string random_string(buf.begin(), buf.end());
    const auto r1 = bech32::Decode(random_string);
    if (r1.hrp.empty()) {
        assert(r1.encoding == bech32::Encoding::INVALID);
        assert(r1.data.empty());
    } else {
        assert(r1.encoding != bech32::Encoding::INVALID);
        const std::string reencoded = bech32::Encode(r1.encoding, r1.hrp, r1.data);
        assert(CaseInsensitiveEqual(random_string, reencoded));
    }

    // make the buffer size valid for encoding
    buf.resize(96);
    std::vector<unsigned char> input;
    ConvertBits<8, 5, true>([&](unsigned char c) { input.push_back(c); }, buf.begin(), buf.end());

    for (auto encoding : {bech32::Encoding::BECH32, bech32::Encoding::BECH32M}) {
        const std::string encoded = bech32::Encode(encoding, "nv", input);
        assert(!encoded.empty());
        const auto r2 = bech32::Decode(encoded);
        assert(r2.encoding == encoding);
        assert(r2.hrp == "nv");
        assert(r2.data == input);
    }
}
