// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <addrdb.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <cassert>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

FUZZ_TARGET(addrdb)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());

    // The point of this code is to exercise all CBanEntry constructors.
    const CBanEntry ban_entry{fuzzed_data_provider.ConsumeIntegral<int64_t>()};
    (void)ban_entry; // currently unused
}
