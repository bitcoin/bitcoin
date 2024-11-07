// Copyright (c) 2024-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/timeoffsets.h>
#include <node/warnings.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/util/setup_common.h>

#include <chrono>
#include <cstdint>
#include <functional>

void initialize_timeoffsets()
{
    static const auto testing_setup = MakeNoLogFileContext<>(ChainType::MAIN);
}

FUZZ_TARGET(timeoffsets, .init = initialize_timeoffsets)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    node::Warnings warnings{};
    TimeOffsets offsets{warnings};
    LIMITED_WHILE(fuzzed_data_provider.remaining_bytes() > 0, 4'000) {
        (void)offsets.Median();
        offsets.Add(std::chrono::seconds{fuzzed_data_provider.ConsumeIntegral<std::chrono::seconds::rep>()});
        offsets.WarnIfOutOfSync();
    }
}
