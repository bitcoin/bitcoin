// Copyright (c) 2009-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <util/bip32.h>

#include <cassert>
#include <cstdint>
#include <vector>

FUZZ_TARGET(parse_hd_keypath)
{
    const std::string keypath_str(buffer.begin(), buffer.end());
    (void)ParseHDKeypath(keypath_str);

    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    KeyPath random_keypath;
    for (const uint32_t i: ConsumeRandomLengthIntegralVector<uint32_t>(fuzzed_data_provider)) {
        random_keypath.emplace_back(i);
    }

    // Roundtrip WriteHDKeypath() and ParseHDKeypath()
    for (const bool apostrophe : {false, true}) {
        const std::string written{WriteHDKeypath(random_keypath, apostrophe)};
        std::optional<KeyPath> roundtrip = ParseHDKeypath(written);
        assert(roundtrip);
        assert(roundtrip == random_keypath);
    }
}
