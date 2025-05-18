// Copyright (c) 2020-2021 The Tortoisecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <cstdint>
#include <string>
#include <vector>

namespace {
template <typename T>
void TestMultiplicationOverflow(FuzzedDataProvider& fuzzed_data_provider)
{
    const T i = fuzzed_data_provider.ConsumeIntegral<T>();
    const T j = fuzzed_data_provider.ConsumeIntegral<T>();
    const bool is_multiplication_overflow_custom = MultiplicationOverflow(i, j);
#ifndef _MSC_VER
    T result_builtin;
    const bool is_multiplication_overflow_builtin = __builtin_mul_overflow(i, j, &result_builtin);
    assert(is_multiplication_overflow_custom == is_multiplication_overflow_builtin);
    if (!is_multiplication_overflow_custom) {
        assert(i * j == result_builtin);
    }
#else
    if (!is_multiplication_overflow_custom) {
        (void)(i * j);
    }
#endif
}
} // namespace

FUZZ_TARGET(multiplication_overflow)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    TestMultiplicationOverflow<int64_t>(fuzzed_data_provider);
    TestMultiplicationOverflow<uint64_t>(fuzzed_data_provider);
    TestMultiplicationOverflow<int32_t>(fuzzed_data_provider);
    TestMultiplicationOverflow<uint32_t>(fuzzed_data_provider);
    TestMultiplicationOverflow<int16_t>(fuzzed_data_provider);
    TestMultiplicationOverflow<uint16_t>(fuzzed_data_provider);
    TestMultiplicationOverflow<char>(fuzzed_data_provider);
    TestMultiplicationOverflow<unsigned char>(fuzzed_data_provider);
    TestMultiplicationOverflow<signed char>(fuzzed_data_provider);
}
