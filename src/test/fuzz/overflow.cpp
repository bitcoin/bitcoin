// Copyright (c) 2025-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <util/check.h>
#include <util/overflow.h>

#include <algorithm>
#include <limits>
#include <optional>

namespace {
//! Test overflow operations for type T using a wider type, W, to verify results.
template <typename T, typename W>
void TestOverflow(FuzzedDataProvider& fuzzed_data_provider)
{
    constexpr auto min{std::numeric_limits<T>::min()};
    constexpr auto max{std::numeric_limits<T>::max()};
    // Range needs to be at least twice as big to allow two numbers to be added without overflowing.
    static_assert(min >= std::numeric_limits<W>::min() / 2);
    static_assert(max <= std::numeric_limits<W>::max() / 2);

    auto widen = [](T value) -> W { return value; };
    auto clamp = [](W value) -> W { return std::clamp<W>(value, min, max); };
    auto check = [](W value) -> std::optional<W> { if (value >= min && value <= max) return value; else return std::nullopt; };

    const T i = fuzzed_data_provider.ConsumeIntegral<T>();
    const T j = fuzzed_data_provider.ConsumeIntegral<T>();
    const unsigned shift = fuzzed_data_provider.ConsumeIntegralInRange<unsigned>(0, std::numeric_limits<W>::digits - std::numeric_limits<T>::digits);

    Assert(clamp(widen(i) + widen(j)) == SaturatingAdd(i, j));
    Assert(check(widen(i) + widen(j)) == CheckedAdd(i, j));

    Assert(clamp(widen(i) << shift) == SaturatingLeftShift(i, shift));
    Assert(check(widen(i) << shift) == CheckedLeftShift(i, shift));
}
} // namespace

FUZZ_TARGET(overflow)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    TestOverflow<int8_t, int64_t>(fuzzed_data_provider);
    TestOverflow<int16_t, int64_t>(fuzzed_data_provider);
    TestOverflow<int32_t, int64_t>(fuzzed_data_provider);
    TestOverflow<uint8_t, uint64_t>(fuzzed_data_provider);
    TestOverflow<uint16_t, uint64_t>(fuzzed_data_provider);
    TestOverflow<uint32_t, uint64_t>(fuzzed_data_provider);
}
