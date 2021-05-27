// Copyright (c) 2019-2020 The Bitcoin Core developers
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

FUZZ_TARGET(bech32)
{
    const std::string random_string(buffer.begin(), buffer.end());
    const auto r1 = bech32::Decode(random_string);
    if (r1.hrp.empty()) {
        assert(r1.encoding == bech32::Encoding::INVALID);
        assert(r1.data.empty());
    } else {
        assert(r1.encoding != bech32::Encoding::INVALID);
        const std::string reencoded = bech32::Encode(r1.encoding, r1.hrp, r1.data);
        assert(CaseInsensitiveEqual(random_string, reencoded));
    }

    std::vector<unsigned char> input;
    ConvertBits<8, 5, true>([&](unsigned char c) { input.push_back(c); }, buffer.begin(), buffer.end());

    if (input.size() + 3 + 6 <= 90) {
        // If it's possible to encode input in Bech32(m) without exceeding the 90-character limit:
        for (auto encoding : {bech32::Encoding::BECH32, bech32::Encoding::BECH32M}) {
            const std::string encoded = bech32::Encode(encoding, "bc", input);
            assert(!encoded.empty());
            const auto r2 = bech32::Decode(encoded);
            assert(r2.encoding == encoding);
            assert(r2.hrp == "bc");
            assert(r2.data == input);
        }
    }
}
