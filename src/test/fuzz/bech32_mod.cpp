// Copyright (c) 2023 The Navcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <blsct/bech32_mod.h>
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

FUZZ_TARGET(bech32_mod)
{
    // create 154-byte buf from the given buffer
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
    const auto r1 = bech32_mod::Decode(random_string);
    if (r1.hrp.empty()) {
        assert(r1.encoding == bech32_mod::Encoding::INVALID);
        assert(r1.data.empty());
    } else {
        assert(r1.encoding != bech32_mod::Encoding::INVALID);
        const std::string reencoded = bech32_mod::Encode(r1.encoding, r1.hrp, r1.data);
        assert(CaseInsensitiveEqual(random_string, reencoded));
    }

    // make the buf valid for encoding
    buf.resize(96);
    std::vector<unsigned char> input;
    ConvertBits<8, 5, true>([&](unsigned char c) { input.push_back(c); }, buf.begin(), buf.end());

    for (auto encoding : {bech32_mod::Encoding::BECH32, bech32_mod::Encoding::BECH32M}) {
        const std::string encoded = bech32_mod::Encode(encoding, "nv", input);
        assert(!encoded.empty());
        const auto r2 = bech32_mod::Decode(encoded);
        assert(r2.encoding == encoding);
        assert(r2.hrp == "nv");
        assert(r2.data == input);
    }
}
