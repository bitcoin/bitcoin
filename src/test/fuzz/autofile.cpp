// Copyright (c) 2020-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

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

FUZZ_TARGET(autofile)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    FuzzedAutoFileProvider fuzzed_auto_file_provider = ConsumeAutoFile(fuzzed_data_provider);
    CAutoFile auto_file = fuzzed_auto_file_provider.open();
    LIMITED_WHILE(fuzzed_data_provider.ConsumeBool(), 10000) {
        CallOneOf(
            fuzzed_data_provider,
            [&] {
                std::array<uint8_t, 4096> arr{};
                try {
                    auto_file.read((char*)arr.data(), fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, 4096));
                } catch (const std::ios_base::failure&) {
                }
            },
            [&] {
                const std::array<uint8_t, 4096> arr{};
                try {
                    auto_file.write((const char*)arr.data(), fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, 4096));
                } catch (const std::ios_base::failure&) {
                }
            },
            [&] {
                try {
                    auto_file.ignore(fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, 4096));
                } catch (const std::ios_base::failure&) {
                }
            },
            [&] {
                auto_file.fclose();
            },
            [&] {
                ReadFromStream(fuzzed_data_provider, auto_file);
            },
            [&] {
                WriteToStream(fuzzed_data_provider, auto_file);
            });
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
