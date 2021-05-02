// Copyright (c) 2019 The XBit Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <util/time.h>

#include <cassert>
#include <cstdint>
#include <string>
#include <vector>

void test_one_input(const std::vector<uint8_t>& buffer)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());

    const int64_t random_time = fuzzed_data_provider.ConsumeIntegral<int64_t>();
    const std::string random_string = fuzzed_data_provider.ConsumeRemainingBytesAsString();

    const std::string iso8601_datetime = FormatISO8601DateTime(random_time);
    const int64_t parsed_time_1 = ParseISO8601DateTime(iso8601_datetime);
    if (random_time >= 0) {
        assert(parsed_time_1 >= 0);
        if (iso8601_datetime.length() == 20) {
            assert(parsed_time_1 == random_time);
        }
    }

    const int64_t parsed_time_2 = ParseISO8601DateTime(random_string);
    assert(parsed_time_2 >= 0);
}
