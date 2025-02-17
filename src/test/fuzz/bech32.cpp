// Copyright (c) 2019-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bech32.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/util/str.h>
#include <util/strencodings.h>

#include <cassert>
#include <cstdint>
#include <string>
#include <vector>

FUZZ_TARGET(bech32_random_decode)
{
    auto limit = bech32::CharLimit::BECH32;
    FuzzedDataProvider fdp(buffer.data(), buffer.size());
    auto random_string = fdp.ConsumeRandomLengthString(limit + 1);
    auto decoded = bech32::Decode(random_string, limit);

    if (decoded.hrp.empty()) {
        assert(decoded.encoding == bech32::Encoding::INVALID);
        assert(decoded.data.empty());
    } else {
        assert(decoded.encoding != bech32::Encoding::INVALID);
        auto reencoded = bech32::Encode(decoded.encoding, decoded.hrp, decoded.data);
        assert(CaseInsensitiveEqual(random_string, reencoded));
    }
}

// https://github.com/bitcoin/bips/blob/master/bip-0173.mediawiki and https://github.com/bitcoin/bips/blob/master/bip-0350.mediawiki
std::string GenerateRandomHRP(FuzzedDataProvider& fdp)
{
    std::string hrp;
    size_t length = fdp.ConsumeIntegralInRange<size_t>(1, 83);
    for (size_t i = 0; i < length; ++i) {
        // Generate lowercase ASCII characters in ([33-126] - ['A'-'Z']) range
        char c = fdp.ConsumeBool()
                 ? fdp.ConsumeIntegralInRange<char>(33, 'A' - 1)
                 : fdp.ConsumeIntegralInRange<char>('Z' + 1, 126);
        hrp += c;
    }
    return hrp;
}

FUZZ_TARGET(bech32_roundtrip)
{
    FuzzedDataProvider fdp(buffer.data(), buffer.size());
    auto hrp = GenerateRandomHRP(fdp);

    auto input_chars = fdp.ConsumeBytes<unsigned char>(fdp.ConsumeIntegralInRange<size_t>(0, 82));
    std::vector<uint8_t> converted_input;
    ConvertBits<8, 5, true>([&](auto c) { converted_input.push_back(c); }, input_chars.begin(), input_chars.end());

    auto size = converted_input.size() + hrp.length() + std::string({bech32::SEPARATOR}).size() + bech32::CHECKSUM_SIZE;
    if (size <= bech32::CharLimit::BECH32) {
        for (auto encoding: {bech32::Encoding::BECH32, bech32::Encoding::BECH32M}) {
            auto encoded = bech32::Encode(encoding, hrp, converted_input);
            assert(!encoded.empty());

            const auto decoded = bech32::Decode(encoded);
            assert(decoded.encoding == encoding);
            assert(decoded.hrp == hrp);
            assert(decoded.data == converted_input);
        }
    }
}