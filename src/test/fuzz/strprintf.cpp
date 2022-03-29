// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <tinyformat.h>
#include <util/strencodings.h>

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <string>
#include <vector>

void test_one_input(const std::vector<uint8_t>& buffer)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    const std::string format_string = fuzzed_data_provider.ConsumeRandomLengthString(64);

    const int digits_in_format_specifier = std::count_if(format_string.begin(), format_string.end(), IsDigit);

    // Avoid triggering the following crash bug:
    // * strprintf("%987654321000000:", 1);
    //
    // Avoid triggering the following OOM bug:
    // * strprintf("%.222222200000000$", 1.1);
    //
    // Upstream bug report: https://github.com/c42f/tinyformat/issues/70
    if (format_string.find("%") != std::string::npos && digits_in_format_specifier >= 7) {
        return;
    }

    // Avoid triggering the following crash bug:
    // * strprintf("%1$*1$*", -11111111);
    //
    // Upstream bug report: https://github.com/c42f/tinyformat/issues/70
    if (format_string.find("%") != std::string::npos && format_string.find("$") != std::string::npos && format_string.find("*") != std::string::npos && digits_in_format_specifier > 0) {
        return;
    }

    // Avoid triggering the following crash bug:
    // * strprintf("%.1s", (char*)nullptr);
    //
    // (void)strprintf(format_string, (char*)nullptr);
    //
    // Upstream bug report: https://github.com/c42f/tinyformat/issues/70

    try {
        (void)strprintf(format_string, (signed char*)nullptr);
    } catch (const tinyformat::format_error&) {
    }
    try {
        (void)strprintf(format_string, (unsigned char*)nullptr);
    } catch (const tinyformat::format_error&) {
    }
    try {
        (void)strprintf(format_string, (void*)nullptr);
    } catch (const tinyformat::format_error&) {
    }
    try {
        (void)strprintf(format_string, (bool*)nullptr);
    } catch (const tinyformat::format_error&) {
    }
    try {
        (void)strprintf(format_string, (float*)nullptr);
    } catch (const tinyformat::format_error&) {
    }
    try {
        (void)strprintf(format_string, (double*)nullptr);
    } catch (const tinyformat::format_error&) {
    }
    try {
        (void)strprintf(format_string, (int16_t*)nullptr);
    } catch (const tinyformat::format_error&) {
    }
    try {
        (void)strprintf(format_string, (uint16_t*)nullptr);
    } catch (const tinyformat::format_error&) {
    }
    try {
        (void)strprintf(format_string, (int32_t*)nullptr);
    } catch (const tinyformat::format_error&) {
    }
    try {
        (void)strprintf(format_string, (uint32_t*)nullptr);
    } catch (const tinyformat::format_error&) {
    }
    try {
        (void)strprintf(format_string, (int64_t*)nullptr);
    } catch (const tinyformat::format_error&) {
    }
    try {
        (void)strprintf(format_string, (uint64_t*)nullptr);
    } catch (const tinyformat::format_error&) {
    }

    try {
        switch (fuzzed_data_provider.ConsumeIntegralInRange(0, 13)) {
        case 0:
            (void)strprintf(format_string, fuzzed_data_provider.ConsumeRandomLengthString(32));
            break;
        case 1:
            (void)strprintf(format_string, fuzzed_data_provider.ConsumeRandomLengthString(32).c_str());
            break;
        case 2:
            (void)strprintf(format_string, fuzzed_data_provider.ConsumeIntegral<signed char>());
            break;
        case 3:
            (void)strprintf(format_string, fuzzed_data_provider.ConsumeIntegral<unsigned char>());
            break;
        case 4:
            (void)strprintf(format_string, fuzzed_data_provider.ConsumeIntegral<char>());
            break;
        case 5:
            (void)strprintf(format_string, fuzzed_data_provider.ConsumeBool());
            break;
        case 6:
            (void)strprintf(format_string, fuzzed_data_provider.ConsumeFloatingPoint<float>());
            break;
        case 7:
            (void)strprintf(format_string, fuzzed_data_provider.ConsumeFloatingPoint<double>());
            break;
        case 8:
            (void)strprintf(format_string, fuzzed_data_provider.ConsumeIntegral<int16_t>());
            break;
        case 9:
            (void)strprintf(format_string, fuzzed_data_provider.ConsumeIntegral<uint16_t>());
            break;
        case 10:
            (void)strprintf(format_string, fuzzed_data_provider.ConsumeIntegral<int32_t>());
            break;
        case 11:
            (void)strprintf(format_string, fuzzed_data_provider.ConsumeIntegral<uint32_t>());
            break;
        case 12:
            (void)strprintf(format_string, fuzzed_data_provider.ConsumeIntegral<int64_t>());
            break;
        case 13:
            (void)strprintf(format_string, fuzzed_data_provider.ConsumeIntegral<uint64_t>());
            break;
        default:
            assert(false);
        }
    } catch (const tinyformat::format_error&) {
    }
}
