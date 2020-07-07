// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <addrdb.h>
#include <optional.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <cassert>
#include <cstdint>
#include <string>
#include <vector>

void test_one_input(const std::vector<uint8_t>& buffer)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());

    const CBanEntry ban_entry = [&] {
        switch (fuzzed_data_provider.ConsumeIntegralInRange<int>(0, 3)) {
        case 0:
            return CBanEntry{fuzzed_data_provider.ConsumeIntegral<int64_t>()};
            break;
        case 1:
            return CBanEntry{fuzzed_data_provider.ConsumeIntegral<int64_t>(), fuzzed_data_provider.PickValueInArray<BanReason>({
                                                                                  BanReason::BanReasonUnknown,
                                                                                  BanReason::BanReasonNodeMisbehaving,
                                                                                  BanReason::BanReasonManuallyAdded,
                                                                              })};
            break;
        case 2: {
            const Optional<CBanEntry> ban_entry = ConsumeDeserializable<CBanEntry>(fuzzed_data_provider);
            if (ban_entry) {
                return *ban_entry;
            }
            break;
        }
        }
        return CBanEntry{};
    }();
    assert(!ban_entry.banReasonToString().empty());
}
