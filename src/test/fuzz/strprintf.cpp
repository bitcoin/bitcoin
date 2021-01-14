// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <tinyformat.h>
#include <util/strencodings.h>
#include <util/translation.h>

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

FUZZ_TARGET(str_printf)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    const std::string format_string = fuzzed_data_provider.ConsumeRandomLengthString(64);
    const bilingual_str bilingual_string{format_string, format_string};

    const int digits_in_format_specifier = std::count_if(format_string.begin(), format_string.end(), IsDigit);

    // Avoid triggering the following crash bug:
    // * strprintf("%987654321000000:", 1);
    //
    // Avoid triggering the following OOM bug:
    // * strprintf("%.222222200000000$", 1.1);
    //
    // Upstream bug report: https://github.com/c42f/tinyformat/issues/70
    if (format_string.find('%') != std::string::npos && digits_in_format_specifier >= 7) {
        return;
    }

    // Avoid triggering the following crash bug:
    // * strprintf("%1$*1$*", -11111111);
    //
    // Upstream bug report: https://github.com/c42f/tinyformat/issues/70
    if (format_string.find('%') != std::string::npos && format_string.find('$') != std::string::npos && format_string.find('*') != std::string::npos && digits_in_format_specifier > 0) {
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
        (void)tinyformat::format(bilingual_string, (signed char*)nullptr);
    } catch (const tinyformat::format_error&) {
    }
    try {
        (void)strprintf(format_string, (unsigned char*)nullptr);
        (void)tinyformat::format(bilingual_string, (unsigned char*)nullptr);
    } catch (const tinyformat::format_error&) {
    }
    try {
        (void)strprintf(format_string, (void*)nullptr);
        (void)tinyformat::format(bilingual_string, (void*)nullptr);
    } catch (const tinyformat::format_error&) {
    }
    try {
        (void)strprintf(format_string, (bool*)nullptr);
        (void)tinyformat::format(bilingual_string, (bool*)nullptr);
    } catch (const tinyformat::format_error&) {
    }
    try {
        (void)strprintf(format_string, (float*)nullptr);
        (void)tinyformat::format(bilingual_string, (float*)nullptr);
    } catch (const tinyformat::format_error&) {
    }
    try {
        (void)strprintf(format_string, (double*)nullptr);
        (void)tinyformat::format(bilingual_string, (double*)nullptr);
    } catch (const tinyformat::format_error&) {
    }
    try {
        (void)strprintf(format_string, (int16_t*)nullptr);
        (void)tinyformat::format(bilingual_string, (int16_t*)nullptr);
    } catch (const tinyformat::format_error&) {
    }
    try {
        (void)strprintf(format_string, (uint16_t*)nullptr);
        (void)tinyformat::format(bilingual_string, (uint16_t*)nullptr);
    } catch (const tinyformat::format_error&) {
    }
    try {
        (void)strprintf(format_string, (int32_t*)nullptr);
        (void)tinyformat::format(bilingual_string, (int32_t*)nullptr);
    } catch (const tinyformat::format_error&) {
    }
    try {
        (void)strprintf(format_string, (uint32_t*)nullptr);
        (void)tinyformat::format(bilingual_string, (uint32_t*)nullptr);
    } catch (const tinyformat::format_error&) {
    }
    try {
        (void)strprintf(format_string, (int64_t*)nullptr);
        (void)tinyformat::format(bilingual_string, (int64_t*)nullptr);
    } catch (const tinyformat::format_error&) {
    }
    try {
        (void)strprintf(format_string, (uint64_t*)nullptr);
        (void)tinyformat::format(bilingual_string, (uint64_t*)nullptr);
    } catch (const tinyformat::format_error&) {
    }

    try {
        CallOneOf(
            fuzzed_data_provider,
            [&] {
                (void)strprintf(format_string, fuzzed_data_provider.ConsumeRandomLengthString(32));
                (void)tinyformat::format(bilingual_string, fuzzed_data_provider.ConsumeRandomLengthString(32));
            },
            [&] {
                (void)strprintf(format_string, fuzzed_data_provider.ConsumeRandomLengthString(32).c_str());
                (void)tinyformat::format(bilingual_string, fuzzed_data_provider.ConsumeRandomLengthString(32).c_str());
            },
            [&] {
                (void)strprintf(format_string, fuzzed_data_provider.ConsumeIntegral<signed char>());
                (void)tinyformat::format(bilingual_string, fuzzed_data_provider.ConsumeIntegral<signed char>());
            },
            [&] {
                (void)strprintf(format_string, fuzzed_data_provider.ConsumeIntegral<unsigned char>());
                (void)tinyformat::format(bilingual_string, fuzzed_data_provider.ConsumeIntegral<unsigned char>());
            },
            [&] {
                (void)strprintf(format_string, fuzzed_data_provider.ConsumeIntegral<char>());
                (void)tinyformat::format(bilingual_string, fuzzed_data_provider.ConsumeIntegral<char>());
            },
            [&] {
                (void)strprintf(format_string, fuzzed_data_provider.ConsumeBool());
                (void)tinyformat::format(bilingual_string, fuzzed_data_provider.ConsumeBool());
            });
    } catch (const tinyformat::format_error&) {
    }

    if (format_string.find('%') != std::string::npos && format_string.find('c') != std::string::npos) {
        // Avoid triggering the following:
        // * strprintf("%c", 1.31783e+38);
        // tinyformat.h:244:36: runtime error: 1.31783e+38 is outside the range of representable values of type 'char'
        return;
    }

    if (format_string.find('%') != std::string::npos && format_string.find('*') != std::string::npos) {
        // Avoid triggering the following:
        // * strprintf("%*", -2.33527e+38);
        // tinyformat.h:283:65: runtime error: -2.33527e+38 is outside the range of representable values of type 'int'
        // * strprintf("%*", -2147483648);
        // tinyformat.h:763:25: runtime error: negation of -2147483648 cannot be represented in type 'int'; cast to an unsigned type to negate this value to itself
        return;
    }

    try {
        CallOneOf(
            fuzzed_data_provider,
            [&] {
                (void)strprintf(format_string, fuzzed_data_provider.ConsumeFloatingPoint<float>());
                (void)tinyformat::format(bilingual_string, fuzzed_data_provider.ConsumeFloatingPoint<float>());
            },
            [&] {
                (void)strprintf(format_string, fuzzed_data_provider.ConsumeFloatingPoint<double>());
                (void)tinyformat::format(bilingual_string, fuzzed_data_provider.ConsumeFloatingPoint<double>());
            },
            [&] {
                (void)strprintf(format_string, fuzzed_data_provider.ConsumeIntegral<int16_t>());
                (void)tinyformat::format(bilingual_string, fuzzed_data_provider.ConsumeIntegral<int16_t>());
            },
            [&] {
                (void)strprintf(format_string, fuzzed_data_provider.ConsumeIntegral<uint16_t>());
                (void)tinyformat::format(bilingual_string, fuzzed_data_provider.ConsumeIntegral<uint16_t>());
            },
            [&] {
                (void)strprintf(format_string, fuzzed_data_provider.ConsumeIntegral<int32_t>());
                (void)tinyformat::format(bilingual_string, fuzzed_data_provider.ConsumeIntegral<int32_t>());
            },
            [&] {
                (void)strprintf(format_string, fuzzed_data_provider.ConsumeIntegral<uint32_t>());
                (void)tinyformat::format(bilingual_string, fuzzed_data_provider.ConsumeIntegral<uint32_t>());
            },
            [&] {
                (void)strprintf(format_string, fuzzed_data_provider.ConsumeIntegral<int64_t>());
                (void)tinyformat::format(bilingual_string, fuzzed_data_provider.ConsumeIntegral<int64_t>());
            },
            [&] {
                (void)strprintf(format_string, fuzzed_data_provider.ConsumeIntegral<uint64_t>());
                (void)tinyformat::format(bilingual_string, fuzzed_data_provider.ConsumeIntegral<uint64_t>());
            });
    } catch (const tinyformat::format_error&) {
    }
}
