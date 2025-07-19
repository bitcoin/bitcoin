// Copyright (c) 2020-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <span.h>
#include <streams.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/util.h>
#include <util/obfuscation.h>

#include <array>
#include <cstddef>
#include <cstdio>
#include <iostream>
#include <vector>

FUZZ_TARGET(autofile)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    FuzzedFileProvider fuzzed_file_provider{fuzzed_data_provider};
    const auto key_bytes{ConsumeFixedLengthByteVector<std::byte>(fuzzed_data_provider, Obfuscation::KEY_SIZE)};
    AutoFile auto_file{
        fuzzed_file_provider.open(),
        Obfuscation{std::span{key_bytes}.first<Obfuscation::KEY_SIZE>()},
    };
    LIMITED_WHILE(fuzzed_data_provider.ConsumeBool(), 100)
    {
        CallOneOf(
            fuzzed_data_provider,
            [&] {
                std::array<std::byte, 4096> arr{};
                try {
                    auto_file.read({arr.data(), fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, 4096)});
                } catch (const std::ios_base::failure&) {
                }
            },
            [&] {
                const std::array<std::byte, 4096> arr{};
                try {
                    auto_file.write({arr.data(), fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, 4096)});
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
                (void)auto_file.fclose();
            },
            [&] {
                ReadFromStream(fuzzed_data_provider, auto_file);
            },
            [&] {
                WriteToStream(fuzzed_data_provider, auto_file);
            });
    }
    (void)auto_file.IsNull();
    if (fuzzed_data_provider.ConsumeBool()) {
        FILE* f = auto_file.release();
        if (f != nullptr) {
            fclose(f);
        }
    } else {
        (void)auto_file.fclose();
    }
}
