// Copyright (c) 2019-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <util/time.h>

#include <cassert>
#include <cstdint>
#include <string>
#include <vector>

FUZZ_TARGET(parse_iso8601)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());

    const int64_t random_time = fuzzed_data_provider.ConsumeIntegral<int32_t>();
    const std::string random_string = fuzzed_data_provider.ConsumeRemainingBytesAsString();

    const std::string iso8601_datetime = FormatISO8601DateTime(random_time);
    (void)FormatISO8601Date(random_time);
    const int64_t parsed_time_1{ParseISO8601DateTime(iso8601_datetime).value()};
    assert(parsed_time_1 == random_time);

    (void)ParseISO8601DateTime(random_string);
}
