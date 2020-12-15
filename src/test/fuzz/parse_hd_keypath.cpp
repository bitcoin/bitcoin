// Copyright (c) 2009-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <util/bip32.h>

#include <cstdint>
#include <vector>

FUZZ_TARGET(parse_hd_keypath)
{
    const std::string keypath_str(buffer.begin(), buffer.end());
    std::vector<uint32_t> keypath;
    (void)ParseHDKeypath(keypath_str, keypath);

    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    const std::vector<uint32_t> random_keypath = ConsumeRandomLengthIntegralVector<uint32_t>(fuzzed_data_provider);
    (void)FormatHDKeypath(random_keypath);
    (void)WriteHDKeypath(random_keypath);
}
