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

FUZZ_TARGET(buffered_file)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    FuzzedFileProvider fuzzed_file_provider = ConsumeFile(fuzzed_data_provider);
    std::optional<CBufferedFile> opt_buffered_file;
    FILE* fuzzed_file = fuzzed_file_provider.open();
    try {
        opt_buffered_file.emplace(fuzzed_file, fuzzed_data_provider.ConsumeIntegralInRange<uint64_t>(0, 4096), fuzzed_data_provider.ConsumeIntegralInRange<uint64_t>(0, 4096), fuzzed_data_provider.ConsumeIntegral<int>(), fuzzed_data_provider.ConsumeIntegral<int>());
    } catch (const std::ios_base::failure&) {
        if (fuzzed_file != nullptr) {
            fclose(fuzzed_file);
        }
    }
    if (opt_buffered_file && fuzzed_file != nullptr) {
        bool setpos_fail = false;
        LIMITED_WHILE(fuzzed_data_provider.ConsumeBool(), 10000) {
            CallOneOf(
                fuzzed_data_provider,
                [&] {
                    std::array<uint8_t, 4096> arr{};
                    try {
                        opt_buffered_file->read((char*)arr.data(), fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, 4096));
                    } catch (const std::ios_base::failure&) {
                    }
                },
                [&] {
                    opt_buffered_file->SetLimit(fuzzed_data_provider.ConsumeIntegralInRange<uint64_t>(0, 4096));
                },
                [&] {
                    if (!opt_buffered_file->SetPos(fuzzed_data_provider.ConsumeIntegralInRange<uint64_t>(0, 4096))) {
                        setpos_fail = true;
                    }
                },
                [&] {
                    if (setpos_fail) {
                        // Calling FindByte(...) after a failed SetPos(...) call may result in an infinite loop.
                        return;
                    }
                    try {
                        opt_buffered_file->FindByte(fuzzed_data_provider.ConsumeIntegral<char>());
                    } catch (const std::ios_base::failure&) {
                    }
                },
                [&] {
                    ReadFromStream(fuzzed_data_provider, *opt_buffered_file);
                });
        }
        opt_buffered_file->GetPos();
        opt_buffered_file->GetType();
        opt_buffered_file->GetVersion();
    }
}
