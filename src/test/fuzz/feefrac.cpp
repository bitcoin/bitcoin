// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <arith_uint256.h>
#include <util/feefrac.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <compare>
#include <cstdint>
#include <iostream>

namespace {

/** The maximum absolute value of an int64_t, as an arith_uint256 (2^63). */
const auto MAX_ABS_INT64 = arith_uint256{1} << 63;

/** Construct an arith_uint256 whose value equals abs(x). */
arith_uint256 Abs256(int64_t x)
{
    if (x >= 0) {
        // For positive numbers, pass through the value.
        return arith_uint256{static_cast<uint64_t>(x)};
    } else if (x > std::numeric_limits<int64_t>::min()) {
        // For negative numbers, negate first.
        return arith_uint256{static_cast<uint64_t>(-x)};
    } else {
        // Special case for x == -2^63 (for which -x results in integer overflow).
        return MAX_ABS_INT64;
    }
}

std::strong_ordering MulCompare(int64_t a1, int64_t a2, int64_t b1, int64_t b2)
{
    // Compute and compare signs.
    int sign_a = (a1 == 0 ? 0 : a1 < 0 ? -1 : 1) * (a2 == 0 ? 0 : a2 < 0 ? -1 : 1);
    int sign_b = (b1 == 0 ? 0 : b1 < 0 ? -1 : 1) * (b2 == 0 ? 0 : b2 < 0 ? -1 : 1);
    if (sign_a != sign_b) return sign_a <=> sign_b;

    // Compute absolute values of products.
    auto mul_abs_a = Abs256(a1) * Abs256(a2), mul_abs_b = Abs256(b1) * Abs256(b2);

    // Compute products of absolute values.
    if (sign_a < 0) {
        return mul_abs_b <=> mul_abs_a;
    } else {
        return mul_abs_a <=> mul_abs_b;
    }
}

} // namespace

FUZZ_TARGET(feefrac)
{
    FuzzedDataProvider provider(buffer.data(), buffer.size());

    int64_t f1 = provider.ConsumeIntegral<int64_t>();
    int32_t s1 = provider.ConsumeIntegral<int32_t>();
    if (s1 == 0) f1 = 0;
    FeeFrac fr1(f1, s1);
    assert(fr1.IsEmpty() == (s1 == 0));

    int64_t f2 = provider.ConsumeIntegral<int64_t>();
    int32_t s2 = provider.ConsumeIntegral<int32_t>();
    if (s2 == 0) f2 = 0;
    FeeFrac fr2(f2, s2);
    assert(fr2.IsEmpty() == (s2 == 0));

    // Feerate comparisons
    auto cmp_feerate = MulCompare(f1, s2, f2, s1);
    assert(FeeRateCompare(fr1, fr2) == cmp_feerate);
    assert((fr1 << fr2) == std::is_lt(cmp_feerate));
    assert((fr1 >> fr2) == std::is_gt(cmp_feerate));

    // Compare with manual invocation of FeeFrac::Mul.
    auto cmp_mul = FeeFrac::Mul(f1, s2) <=> FeeFrac::Mul(f2, s1);
    assert(cmp_mul == cmp_feerate);

    // Same, but using FeeFrac::MulFallback.
    auto cmp_fallback = FeeFrac::MulFallback(f1, s2) <=> FeeFrac::MulFallback(f2, s1);
    assert(cmp_fallback == cmp_feerate);

    // Total order comparisons
    auto cmp_total = std::is_eq(cmp_feerate) ? (s2 <=> s1) : cmp_feerate;
    assert((fr1 <=> fr2) == cmp_total);
    assert((fr1 < fr2) == std::is_lt(cmp_total));
    assert((fr1 > fr2) == std::is_gt(cmp_total));
    assert((fr1 <= fr2) == std::is_lteq(cmp_total));
    assert((fr1 >= fr2) == std::is_gteq(cmp_total));
    assert((fr1 == fr2) == std::is_eq(cmp_total));
    assert((fr1 != fr2) == std::is_neq(cmp_total));
}
