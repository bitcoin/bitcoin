// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <span.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

FUZZ_TARGET(span)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());

    std::string str = fuzzed_data_provider.ConsumeBytesAsString(32);
    const std::span<const char> span{str};
    (void)span.data();
    (void)span.begin();
    (void)span.end();
    if (span.size() > 0) {
        const std::ptrdiff_t idx = fuzzed_data_provider.ConsumeIntegralInRange<std::ptrdiff_t>(0U, span.size() - 1U);
        (void)span.first(idx);
        (void)span.last(idx);
        (void)span.subspan(idx);
        (void)span.subspan(idx, span.size() - idx);
        (void)span[idx];
    }
}
