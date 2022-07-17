// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <optional.h>
#include <streams.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <array>
#include <cstdint>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

void test_one_input(const std::vector<uint8_t>& buffer)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    FuzzedAutoFileProvider fuzzed_auto_file_provider = ConsumeAutoFile(fuzzed_data_provider);
    CAutoFile auto_file = fuzzed_auto_file_provider.open();
    while (fuzzed_data_provider.ConsumeBool()) {
        switch (fuzzed_data_provider.ConsumeIntegralInRange<int>(0, 5)) {
        case 0: {
            std::array<uint8_t, 4096> arr{};
            try {
                auto_file.read((char*)arr.data(), fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, 4096));
            } catch (const std::ios_base::failure&) {
            }
            break;
        }
        case 1: {
            const std::array<uint8_t, 4096> arr{};
            try {
                auto_file.write((const char*)arr.data(), fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, 4096));
            } catch (const std::ios_base::failure&) {
            }
            break;
        }
        case 2: {
            try {
                auto_file.ignore(fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, 4096));
            } catch (const std::ios_base::failure&) {
            }
            break;
        }
        case 3: {
            auto_file.fclose();
            break;
        }
        case 4: {
            ReadFromStream(fuzzed_data_provider, auto_file);
            break;
        }
        case 5: {
            WriteToStream(fuzzed_data_provider, auto_file);
            break;
        }
        }
    }
    (void)auto_file.Get();
    (void)auto_file.GetType();
    (void)auto_file.GetVersion();
    (void)auto_file.IsNull();
    if (fuzzed_data_provider.ConsumeBool()) {
        FILE* f = auto_file.release();
        if (f != nullptr) {
            fclose(f);
        }
    }
}
